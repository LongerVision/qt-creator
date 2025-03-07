/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "CppDocument.h"
#include "FastPreprocessor.h"
#include "LookupContext.h"
#include "Overview.h"

#include <cplusplus/Bind.h>
#include <cplusplus/Control.h>
#include <cplusplus/TranslationUnit.h>
#include <cplusplus/DiagnosticClient.h>
#include <cplusplus/Literals.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/Names.h>
#include <cplusplus/AST.h>
#include <cplusplus/ASTPatternBuilder.h>
#include <cplusplus/ASTMatcher.h>
#include <cplusplus/Scope.h>
#include <cplusplus/SymbolVisitor.h>
#include <cplusplus/NameVisitor.h>
#include <cplusplus/TypeVisitor.h>
#include <cplusplus/CoreTypes.h>

#include <QBitArray>
#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QFutureInterface>
#include <QStack>

/*!
    \namespace CPlusPlus
    The namespace for C++ related tools.
*/

using namespace CPlusPlus;

namespace {

class LastVisibleSymbolAt: protected SymbolVisitor
{
    Symbol *root;
    int line;
    int column;
    Symbol *symbol;

public:
    LastVisibleSymbolAt(Symbol *root)
        : root(root), line(0), column(0), symbol(nullptr) {}

    Symbol *operator()(int line, int column)
    {
        this->line = line;
        this->column = column;
        this->symbol = nullptr;
        accept(root);
        if (! symbol)
            symbol = root;
        return symbol;
    }

protected:
    bool preVisit(Symbol *s) final
    {
        if (s->line() < line || (s->line() == line && s->column() <= column)) {
            // skip blocks
            if (!s->asBlock())
                symbol = s;
            return true;
        }

        return false;
    }
};

class FindScopeAt: protected SymbolVisitor
{
    TranslationUnit *_unit;
    int _line;
    int _column;
    Scope *_scope;

public:
    /** line and column should be 1-based */
    FindScopeAt(TranslationUnit *unit, int line, int column)
        : _unit(unit), _line(line), _column(column), _scope(nullptr) {}

    Scope *operator()(Symbol *symbol)
    {
        accept(symbol);
        return _scope;
    }

protected:
    bool process(Scope *symbol)
    {
        if (! _scope) {
            Scope *scope = symbol;

            for (int i = 0; i < scope->memberCount(); ++i) {
                accept(scope->memberAt(i));

                if (_scope)
                    return false;
            }

            int startLine, startColumn;
            _unit->getPosition(scope->startOffset(), &startLine, &startColumn);

            if (_line > startLine || (_line == startLine && _column >= startColumn)) {
                int endLine, endColumn;
                _unit->getPosition(scope->endOffset(), &endLine, &endColumn);

                if (_line < endLine || (_line == endLine && _column < endColumn))
                    _scope = scope;
            }
        }

        return false;
    }

    using SymbolVisitor::visit;

    bool preVisit(Symbol *) override
    { return ! _scope; }

    bool visit(UsingNamespaceDirective *) override { return false; }
    bool visit(UsingDeclaration *) override { return false; }
    bool visit(NamespaceAlias *) override { return false; }
    bool visit(Declaration *) override { return false; }
    bool visit(Argument *) override { return false; }
    bool visit(TypenameArgument *) override { return false; }
    bool visit(BaseClass *) override { return false; }
    bool visit(ForwardClassDeclaration *) override { return false; }

    bool visit(Enum *symbol) override
    { return process(symbol); }

    bool visit(Function *symbol) override
    { return process(symbol); }

    bool visit(Namespace *symbol) override
    { return process(symbol); }

    bool visit(Class *symbol) override
    { return process(symbol); }

    bool visit(Block *symbol) override
    { return process(symbol); }

    bool visit(Template *symbol) override
    {
        if (Symbol *decl = symbol->declaration()) {
            if (decl->isFunction() || decl->isClass() || decl->isDeclaration())
                return process(symbol);
        }
        return true;
    }

    // Objective-C
    bool visit(ObjCBaseClass *) override { return false; }
    bool visit(ObjCBaseProtocol *) override { return false; }
    bool visit(ObjCForwardClassDeclaration *) override { return false; }
    bool visit(ObjCForwardProtocolDeclaration *) override { return false; }
    bool visit(ObjCPropertyDeclaration *) override { return false; }

    bool visit(ObjCClass *symbol) override
    { return process(symbol); }

    bool visit(ObjCProtocol *symbol) override
    { return process(symbol); }

    bool visit(ObjCMethod *symbol) override
    { return process(symbol); }
};


#define DO_NOT_DUMP_ALL_PARSER_ERRORS

class DocumentDiagnosticClient : public DiagnosticClient
{
    enum { MAX_MESSAGE_COUNT = 10 };

public:
    DocumentDiagnosticClient(Document *doc, QList<Document::DiagnosticMessage> *messages)
        : doc(doc),
          messages(messages),
          errorCount(0)
    { }

    void report(int level,
                const StringLiteral *fileId,
                int line, int column,
                const char *format, va_list ap) override
    {
        if (level == Error) {
            ++errorCount;

#ifdef DO_NOT_DUMP_ALL_PARSER_ERRORS
            if (errorCount >= MAX_MESSAGE_COUNT)
                return; // ignore the error
#endif // DO_NOT_DUMP_ALL_PARSER_ERRORS
        }

        const QString fileName = QString::fromUtf8(fileId->chars(), fileId->size());

        if (fileName != doc->fileName())
            return;

        const QString message = QString::vasprintf(format, ap);

#ifndef DO_NOT_DUMP_ALL_PARSER_ERRORS
        {
            const char *levelStr = "Unknown level";
            if (level == Document::DiagnosticMessage::Warning) levelStr = "Warning";
            if (level == Document::DiagnosticMessage::Error) levelStr = "Error";
            if (level == Document::DiagnosticMessage::Fatal) levelStr = "Fatal";
            qDebug("%s:%u:%u: %s: %s", fileId->chars(), line, column, levelStr, message.toUtf8().constData());
        }
#endif // DO_NOT_DUMP_ALL_PARSER_ERRORS

        Document::DiagnosticMessage m(convertLevel(level), doc->fileName(),
                                      line, column, message);
        messages->append(m);
    }

    static int convertLevel(int level) {
        switch (level) {
            case Warning: return Document::DiagnosticMessage::Warning;
            case Error:   return Document::DiagnosticMessage::Error;
            case Fatal:   return Document::DiagnosticMessage::Fatal;
            default:      return Document::DiagnosticMessage::Error;
        }
    }

private:
    Document *doc;
    QList<Document::DiagnosticMessage> *messages;
    int errorCount;
};

} // anonymous namespace


Document::Document(const QString &fileName)
    : _fileName(QDir::cleanPath(fileName)),
      _globalNamespace(nullptr),
      _revision(0),
      _editorRevision(0),
      _checkMode(0)
{
    _control = new Control();

    _control->setDiagnosticClient(new DocumentDiagnosticClient(this, &_diagnosticMessages));

    const QByteArray localFileName = fileName.toUtf8();
    const StringLiteral *fileId = _control->stringLiteral(localFileName.constData(),
                                                                      localFileName.size());
    _translationUnit = new TranslationUnit(_control, fileId);
    _translationUnit->setLanguageFeatures(LanguageFeatures::defaultFeatures());
}

Document::~Document()
{
    delete _translationUnit;
    _translationUnit = nullptr;
    if (_control) {
        delete _control->diagnosticClient();
        delete _control;
    }
    _control = nullptr;
}

Control *Document::control() const
{
    return _control;
}

Control *Document::swapControl(Control *newControl)
{
    if (newControl) {
        const StringLiteral *fileId = newControl->stringLiteral(_translationUnit->fileId()->chars(),
                                                                _translationUnit->fileId()->size());
        const auto newTranslationUnit = new TranslationUnit(newControl, fileId);
        newTranslationUnit->setLanguageFeatures(_translationUnit->languageFeatures());
        delete _translationUnit;
        _translationUnit = newTranslationUnit;
    } else {
        delete _translationUnit;
        _translationUnit = nullptr;
    }

    Control *oldControl = _control;
    _control = newControl;
    return oldControl;
}

unsigned Document::revision() const
{
    return _revision;
}

void Document::setRevision(unsigned revision)
{
    _revision = revision;
}

unsigned Document::editorRevision() const
{
    return _editorRevision;
}

void Document::setEditorRevision(unsigned editorRevision)
{
    _editorRevision = editorRevision;
}

QDateTime Document::lastModified() const
{
    return _lastModified;
}

void Document::setLastModified(const QDateTime &lastModified)
{
    _lastModified = lastModified;
}

QString Document::fileName() const
{
    return _fileName;
}

QStringList Document::includedFiles() const
{
    QStringList files;
    for (const Include &i : qAsConst(_resolvedIncludes))
        files.append(i.resolvedFileName());
    files.removeDuplicates();
    return files;
}

// This assumes to be called with a QDir::cleanPath cleaned fileName.
void Document::addIncludeFile(const Document::Include &include)
{
    if (include.resolvedFileName().isEmpty())
        _unresolvedIncludes.append(include);
    else
        _resolvedIncludes.append(include);
}

void Document::appendMacro(const Macro &macro)
{
    _definedMacros.append(macro);
}

void Document::addMacroUse(const Macro &macro,
                           int bytesOffset, int bytesLength,
                           int utf16charsOffset, int utf16charLength,
                           int beginLine,
                           const QVector<MacroArgumentReference> &actuals)
{
    MacroUse use(macro,
                 bytesOffset, bytesOffset + bytesLength,
                 utf16charsOffset, utf16charsOffset + utf16charLength,
                 beginLine);

    for (const MacroArgumentReference &actual : actuals) {
        const Block arg(actual.bytesOffset(),
                        actual.bytesOffset() + actual.bytesLength(),
                        actual.utf16charsOffset(),
                        actual.utf16charsOffset() + actual.utf16charsLength());
        use.addArgument(arg);
    }

    _macroUses.append(use);
}

void Document::addUndefinedMacroUse(const QByteArray &name,
                                    int bytesOffset, int utf16charsOffset)
{
    QByteArray copy(name.data(), name.size());
    UndefinedMacroUse use(copy, bytesOffset, utf16charsOffset);
    _undefinedMacroUses.append(use);
}

/*!
    \class Document::MacroUse
    \brief The MacroUse class represents the usage of a macro in a
    \l {Document}.
    \sa Document::UndefinedMacroUse
*/

/*!
    \class Document::UndefinedMacroUse
    \brief The UndefinedMacroUse class represents a macro that was looked for,
    but not found.

    Holds data about the reference to a macro in an \tt{#ifdef} or \tt{#ifndef}
    or argument to the \tt{defined} operator inside an \tt{#if} or \tt{#elif} that does
    not exist.

    \sa Document::undefinedMacroUses(), Document::MacroUse, Macro
*/

/*!
    \fn QByteArray Document::UndefinedMacroUse::name() const

    Returns the name of the macro that was not found.
*/

/*!
    \fn QList<UndefinedMacroUse> Document::undefinedMacroUses() const

    Returns a list of referenced but undefined macros.

    \sa Document::macroUses(), Document::definedMacros(), Macro
*/

/*!
    \fn QList<MacroUse> Document::macroUses() const

    Returns a list of macros used.

    \sa Document::undefinedMacroUses(), Document::definedMacros(), Macro
*/

/*!
    \fn QList<Macro> Document::definedMacros() const

    Returns the list of macros defined.

    \sa Document::macroUses(), Document::undefinedMacroUses()
*/

TranslationUnit *Document::translationUnit() const
{
    return _translationUnit;
}

bool Document::skipFunctionBody() const
{
    return _translationUnit->skipFunctionBody();
}

void Document::setSkipFunctionBody(bool skipFunctionBody)
{
    _translationUnit->setSkipFunctionBody(skipFunctionBody);
}

int Document::globalSymbolCount() const
{
    if (! _globalNamespace)
        return 0;

    return _globalNamespace->memberCount();
}

Symbol *Document::globalSymbolAt(int index) const
{
    return _globalNamespace->memberAt(index);
}

Namespace *Document::globalNamespace() const
{
    return _globalNamespace;
}

void Document::setGlobalNamespace(Namespace *globalNamespace)
{
    _globalNamespace = globalNamespace;
}

/*!
 * Extract the function name including scope at the given position.
 *
 * Note that a function (scope) starts at the name of that function, not at the return type. The
 * implication is that this function will return an empty string when the line/column is on the
 * return type.
 *
 * \param line the line number, starting with line 1
 * \param column the column number, starting with column 1
 * \param lineOpeningDeclaratorParenthesis optional output parameter, the line of the opening
          parenthesis of the declarator starting with 1
 * \param lineClosingBrace optional output parameter, the line of the closing brace starting with 1
 */
QString Document::functionAt(int line, int column, int *lineOpeningDeclaratorParenthesis,
                             int *lineClosingBrace) const
{
    if (line < 1 || column < 1)
        return QString();

    Symbol *symbol = lastVisibleSymbolAt(line, column);
    if (!symbol)
        return QString();

    // Find the enclosing function scope (which might be several levels up, or we might be standing
    // on it)
    Scope *scope = symbol->asScope();
    if (!scope)
        scope = symbol->enclosingScope();

    while (scope && !scope->isFunction() )
        scope = scope->enclosingScope();

    if (!scope)
        return QString();

    // We found the function scope
    if (lineOpeningDeclaratorParenthesis)
        translationUnit()->getPosition(scope->startOffset(), lineOpeningDeclaratorParenthesis);

    if (lineClosingBrace)
        translationUnit()->getPosition(scope->endOffset(), lineClosingBrace);

    const QList<const Name *> fullyQualifiedName = LookupContext::fullyQualifiedName(scope);
    return Overview().prettyName(fullyQualifiedName);
}

Scope *Document::scopeAt(int line, int column)
{
    FindScopeAt findScopeAt(_translationUnit, line, column);
    if (Scope *scope = findScopeAt(_globalNamespace))
        return scope;
    return globalNamespace();
}

Symbol *Document::lastVisibleSymbolAt(int line, int column) const
{
    LastVisibleSymbolAt lastVisibleSymbolAt(globalNamespace());
    return lastVisibleSymbolAt(line, column);
}

const Macro *Document::findMacroDefinitionAt(int line) const
{
    for (const Macro &macro : qAsConst(_definedMacros)) {
        if (macro.line() == line)
            return &macro;
    }
    return nullptr;
}

const Document::MacroUse *Document::findMacroUseAt(int utf16charsOffset) const
{
    for (const Document::MacroUse &use : qAsConst(_macroUses)) {
        if (use.containsUtf16charOffset(utf16charsOffset)
                && (utf16charsOffset < use.utf16charsBegin() + use.macro().nameToQString().size())) {
            return &use;
        }
    }
    return nullptr;
}

const Document::UndefinedMacroUse *Document::findUndefinedMacroUseAt(int utf16charsOffset) const
{
    for (const Document::UndefinedMacroUse &use : qAsConst(_undefinedMacroUses)) {
        if (use.containsUtf16charOffset(utf16charsOffset)
                && (utf16charsOffset < use.utf16charsBegin()
                    + QString::fromUtf8(use.name(), use.name().size()).length()))
            return &use;
    }
    return nullptr;
}

Document::Ptr Document::create(const QString &fileName)
{
    Document::Ptr doc(new Document(fileName));
    return doc;
}

QByteArray Document::utf8Source() const
{ return _source; }

void Document::setUtf8Source(const QByteArray &source)
{
    _source = source;
    _translationUnit->setSource(_source.constBegin(), _source.size());
}

LanguageFeatures Document::languageFeatures() const
{
    if (TranslationUnit *tu = translationUnit())
        return tu->languageFeatures();
    return LanguageFeatures::defaultFeatures();
}

void Document::setLanguageFeatures(LanguageFeatures features)
{
    if (TranslationUnit *tu = translationUnit())
        tu->setLanguageFeatures(features);
}

void Document::startSkippingBlocks(int utf16charsOffset)
{
    _skippedBlocks.append(Block(0, 0, utf16charsOffset, 0));
}

void Document::stopSkippingBlocks(int utf16charsOffset)
{
    if (_skippedBlocks.isEmpty())
        return;

    int start = _skippedBlocks.back().utf16charsBegin();
    if (start > utf16charsOffset)
        _skippedBlocks.removeLast(); // Ignore this block, it's invalid.
    else
        _skippedBlocks.back() = Block(0, 0, start, utf16charsOffset);
}

bool Document::isTokenized() const
{
    return _translationUnit->isTokenized();
}

void Document::tokenize()
{
    _translationUnit->tokenize();
}

bool Document::isParsed() const
{
    return _translationUnit->isParsed();
}

bool Document::parse(ParseMode mode)
{
    TranslationUnit::ParseMode m = TranslationUnit::ParseTranlationUnit;
    switch (mode) {
    case ParseTranlationUnit:
        m = TranslationUnit::ParseTranlationUnit;
        break;

    case ParseDeclaration:
        m = TranslationUnit::ParseDeclaration;
        break;

    case ParseExpression:
        m = TranslationUnit::ParseExpression;
        break;

    case ParseDeclarator:
        m = TranslationUnit::ParseDeclarator;
        break;

    case ParseStatement:
        m = TranslationUnit::ParseStatement;
        break;

    default:
        break;
    }

    return _translationUnit->parse(m);
}

void Document::check(CheckMode mode)
{
    Q_ASSERT(!_globalNamespace);

    _checkMode = mode;

    if (! isParsed())
        parse();

    _globalNamespace = _control->newNamespace(0);
    Bind semantic(_translationUnit);
    if (mode == FastCheck)
        semantic.setSkipFunctionBodies(true);

    if (! _translationUnit->ast())
        return; // nothing to do.

    if (TranslationUnitAST *ast = _translationUnit->ast()->asTranslationUnit())
        semantic(ast, _globalNamespace);
    else if (StatementAST *ast = _translationUnit->ast()->asStatement())
        semantic(ast, _globalNamespace);
    else if (ExpressionAST *ast = _translationUnit->ast()->asExpression())
        semantic(ast, _globalNamespace);
    else if (DeclarationAST *ast = translationUnit()->ast()->asDeclaration())
        semantic(ast, _globalNamespace);
}

void Document::keepSourceAndAST()
{
    _keepSourceAndASTCount.ref();
}

void Document::releaseSourceAndAST()
{
    if (!_keepSourceAndASTCount.deref()) {
        _source.clear();
        _translationUnit->release();
    }
}

bool Document::DiagnosticMessage::operator==(const Document::DiagnosticMessage &other) const
{
    return
            _line == other._line &&
            _column == other._column &&
            _length == other._length &&
            _level == other._level &&
            _fileName == other._fileName &&
            _text == other._text;
}

bool Document::DiagnosticMessage::operator!=(const Document::DiagnosticMessage &other) const
{
    return !operator==(other);
}

Snapshot::Snapshot()
{
}

Snapshot::~Snapshot()
{
}

int Snapshot::size() const
{
    return _documents.size();
}

bool Snapshot::isEmpty() const
{
    return _documents.isEmpty();
}

Snapshot::const_iterator Snapshot::find(const Utils::FilePath &fileName) const
{
    return _documents.find(fileName);
}

void Snapshot::remove(const Utils::FilePath &fileName)
{
    _documents.remove(fileName);
}

bool Snapshot::contains(const Utils::FilePath &fileName) const
{
    return _documents.contains(fileName);
}

void Snapshot::insert(Document::Ptr doc)
{
    if (doc) {
        _documents.insert(Utils::FilePath::fromString(doc->fileName()), doc);
        m_deps.files.clear(); // Will trigger re-build when accessed.
    }
}

static QList<Macro> macrosDefinedUntilLine(const QList<Macro> &macros, int line)
{
    QList<Macro> filtered;

    for (const Macro &macro : macros) {
        if (macro.line() <= line)
            filtered.append(macro);
        else
            break;
    }

    return filtered;
}

Document::Ptr Snapshot::preprocessedDocument(const QByteArray &source,
                                             const Utils::FilePath &fileName,
                                             int withDefinedMacrosFromDocumentUntilLine) const
{
    Document::Ptr newDoc = Document::create(fileName.toString());
    if (Document::Ptr thisDocument = document(fileName)) {
        newDoc->_revision = thisDocument->_revision;
        newDoc->_editorRevision = thisDocument->_editorRevision;
        newDoc->_lastModified = thisDocument->_lastModified;
        newDoc->_resolvedIncludes = thisDocument->_resolvedIncludes;
        newDoc->_unresolvedIncludes = thisDocument->_unresolvedIncludes;
        newDoc->setLanguageFeatures(thisDocument->languageFeatures());
        if (withDefinedMacrosFromDocumentUntilLine != -1) {
            newDoc->_definedMacros = macrosDefinedUntilLine(thisDocument->_definedMacros,
                                                            withDefinedMacrosFromDocumentUntilLine);
        }
    }

    FastPreprocessor pp(*this);
    const bool mergeDefinedMacrosOfDocument = !newDoc->_definedMacros.isEmpty();
    const QByteArray preprocessedCode = pp.run(newDoc, source, mergeDefinedMacrosOfDocument);
    newDoc->setUtf8Source(preprocessedCode);
    return newDoc;
}

Document::Ptr Snapshot::documentFromSource(const QByteArray &preprocessedCode,
                                           const QString &fileName) const
{
    Document::Ptr newDoc = Document::create(fileName);

    if (Document::Ptr thisDocument = document(fileName)) {
        newDoc->_revision = thisDocument->_revision;
        newDoc->_editorRevision = thisDocument->_editorRevision;
        newDoc->_lastModified = thisDocument->_lastModified;
        newDoc->_resolvedIncludes = thisDocument->_resolvedIncludes;
        newDoc->_unresolvedIncludes = thisDocument->_unresolvedIncludes;
        newDoc->_definedMacros = thisDocument->_definedMacros;
        newDoc->_macroUses = thisDocument->_macroUses;
        newDoc->setLanguageFeatures(thisDocument->languageFeatures());
    }

    newDoc->setUtf8Source(preprocessedCode);
    return newDoc;
}

QSet<QString> Snapshot::allIncludesForDocument(const QString &fileName) const
{
    QSet<QString> result;

    QStack<QString> files;
    files.push(fileName);

    while (!files.isEmpty()) {
        QString file = files.pop();
        if (Document::Ptr doc = document(file)) {
            const QStringList includedFiles = doc->includedFiles();
            for (const QString &inc : includedFiles) {
                if (!result.contains(inc)) {
                    result.insert(inc);
                    files.push(inc);
                }
            }
        }
    }

    return result;
}

QList<Snapshot::IncludeLocation> Snapshot::includeLocationsOfDocument(const QString &fileName) const
{
    QList<IncludeLocation> result;
    for (const_iterator cit = begin(), citEnd = end(); cit != citEnd; ++cit) {
        const Document::Ptr doc = cit.value();
        const QList<Document::Include> includeFiles = doc->resolvedIncludes();
        for (const Document::Include &includeFile : includeFiles) {
            if (includeFile.resolvedFileName() == fileName)
                result.append(qMakePair(doc, includeFile.line()));
        }
    }
    return result;
}

Utils::FilePaths Snapshot::filesDependingOn(const Utils::FilePath &fileName) const
{
    updateDependencyTable();
    return m_deps.filesDependingOn(fileName);
}

void Snapshot::updateDependencyTable() const
{
    QFutureInterfaceBase futureInterface;
    updateDependencyTable(futureInterface);
}

void Snapshot::updateDependencyTable(QFutureInterfaceBase &futureInterface) const
{
    if (m_deps.files.isEmpty())
        m_deps.build(futureInterface, *this);
}

bool Snapshot::operator==(const Snapshot &other) const
{
    return _documents == other._documents;
}

Document::Ptr Snapshot::document(const Utils::FilePath &fileName) const
{
    return _documents.value(fileName);
}

Snapshot Snapshot::simplified(Document::Ptr doc) const
{
    Snapshot snapshot;

    if (doc) {
        snapshot.insert(doc);
        const QSet<QString> fileNames = allIncludesForDocument(doc->fileName());
        for (const QString &fileName : fileNames)
            if (Document::Ptr inc = document(fileName))
                snapshot.insert(inc);
    }

    return snapshot;
}
