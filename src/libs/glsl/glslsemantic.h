/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/
#ifndef GLSLSEMANTIC_H
#define GLSLSEMANTIC_H

#include "glslastvisitor.h"

namespace GLSL {

class GLSL_EXPORT Semantic: protected Visitor
{
public:
    Semantic(Engine *engine);
    virtual ~Semantic();

    void expression(ExpressionAST *ast);
    void statement(StatementAST *ast);
    const Type *type(TypeAST *ast);
    void declaration(DeclarationAST *ast);
    void translationUnit(TranslationUnitAST *ast);
    void functionIdentifier(FunctionIdentifierAST *ast);
    void field(StructTypeAST::Field *ast);

protected:
    virtual bool visit(TranslationUnitAST *ast);
    virtual bool visit(FunctionIdentifierAST *ast);
    virtual bool visit(StructTypeAST::Field *ast);

    // expressions
    virtual bool visit(IdentifierExpressionAST *ast);
    virtual bool visit(LiteralExpressionAST *ast);
    virtual bool visit(BinaryExpressionAST *ast);
    virtual bool visit(UnaryExpressionAST *ast);
    virtual bool visit(TernaryExpressionAST *ast);
    virtual bool visit(AssignmentExpressionAST *ast);
    virtual bool visit(MemberAccessExpressionAST *ast);
    virtual bool visit(FunctionCallExpressionAST *ast);
    virtual bool visit(DeclarationExpressionAST *ast);

    // statements
    virtual bool visit(ExpressionStatementAST *ast);
    virtual bool visit(CompoundStatementAST *ast);
    virtual bool visit(IfStatementAST *ast);
    virtual bool visit(WhileStatementAST *ast);
    virtual bool visit(DoStatementAST *ast);
    virtual bool visit(ForStatementAST *ast);
    virtual bool visit(JumpStatementAST *ast);
    virtual bool visit(ReturnStatementAST *ast);
    virtual bool visit(SwitchStatementAST *ast);
    virtual bool visit(CaseLabelStatementAST *ast);
    virtual bool visit(DeclarationStatementAST *ast);

    // types
    virtual bool visit(BasicTypeAST *ast);
    virtual bool visit(NamedTypeAST *ast);
    virtual bool visit(ArrayTypeAST *ast);
    virtual bool visit(StructTypeAST *ast);
    virtual bool visit(QualifiedTypeAST *ast);

    // declarations
    virtual bool visit(PrecisionDeclarationAST *ast);
    virtual bool visit(ParameterDeclarationAST *ast);
    virtual bool visit(VariableDeclarationAST *ast);
    virtual bool visit(TypeDeclarationAST *ast);
    virtual bool visit(TypeAndVariableDeclarationAST *ast);
    virtual bool visit(InvariantDeclarationAST *ast);
    virtual bool visit(InitDeclarationAST *ast);
    virtual bool visit(FunctionDeclarationAST *ast);

private:
    Engine *_engine;
    const Type *_type;
};

} // namespace GLSL

#endif // GLSLSEMANTIC_H