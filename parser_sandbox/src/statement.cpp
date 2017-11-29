#include "statement.hpp"

namespace occa {
  namespace lang {
    statement_t::statement_t(context_t &context_) :
      up(NULL),
      context(context_),
      scope(context_) {
      addRef();
    }

    statement_t::~statement_t() {}

    std::string statement_t::toString() const {
      std::stringstream ss;
      printer_t pout(ss);
      print(pout);
      return ss.str();
    }

    statement_t::operator std::string() const {
      return toString();
    }

    int statement_t::type() const {
      return statementType::none;
    }

    bool statement_t::hasScope() const {
      return false;
    }

    directiveStatement_t statement_t::newDirectiveStatement(macro_t &macro) {
      return directiveStatement_t(context, macro);
    }

    blockStatement_t statement_t::newBlockStatement() {
      return blockStatement_t(context);
    }

    typeDeclStatement_t statement_t::newTypeDeclarationStatement(declarationType_t &declType_) {
      return typeDeclStatement_t(context, declType_);
    }

    classAccessStatement_t statement_t::newClassAccessStatement(const int access_) {
      return classAccessStatement_t(context, access_);
    }

    expressionStatement_t statement_t::newExpressionStatement() {
      return expressionStatement_t(context);
    }

    declarationStatement_t statement_t::newDeclarationStatement() {
      return declarationStatement_t(context);
    }

    gotoStatement_t statement_t::  newGotoStatement(const std::string &name_) {
      return gotoStatement_t(context, name_);
    }

    gotoLabelStatement_t statement_t::newGotoLabelStatement(const std::string &name_) {
      return gotoLabelStatement_t(context, name_);
    }

    namespaceStatement_t statement_t::newNamespaceStatement(const std::string &name_) {
      return namespaceStatement_t(context, name_);
    }

    whileStatement_t statement_t::newWhileStatement(statement_t &check_) {
      return whileStatement_t(context, check_);
    }

    forStatement_t statement_t::newForStatement(statement_t &init_,
                                                statement_t &check_,
                                                statement_t &update_) {
      return forStatement_t(context, init_, check_, update_);
    }

    switchStatement_t statement_t::newSwitchStatement(statement_t &value_) {
      return switchStatement_t(context, value_);
    }

    caseStatement_t statement_t::newCaseStatement(statement_t &value_) {
      return caseStatement_t(context, value_);
    }

    void statement_t::print() const {
      std::cout << toString();
    }

    //---[ Directive ]--------------------
    directiveStatement_t::directiveStatement_t(context_t &context_,
                                               macro_t &macro_) :
      statement_t(context_),
      macro(macro_) {}

    statement_t& directiveStatement_t::clone() const {
      return *(new directiveStatement_t(context, macro));
    }

    void directiveStatement_t::print(printer_t &pout) const {
      pout << macro.toString() << '\n';
    }
    //====================================

    //---[ Block ]------------------------
    blockStatement_t::blockStatement_t(context_t &context_) :
      statement_t(context_) {}

    statement_t& blockStatement_t::clone() const {
      blockStatement_t &s = *(new blockStatement_t(context));
      const int childCount = (int) children.size();
      for (int i = 0; i < childCount; ++i) {
        s.addChild(children[i]->clone());
      }
      return s;
    }

    int blockStatement_t::type() const {
      return statementType::block;
    }

    bool blockStatement_t::hasScope() const {
      return true;
    }

    void blockStatement_t::addChild(statement_t &child) {
      children.push_back(&child);
      child.up = this;
    }

    void blockStatement_t::clearChildren() {
      const int count = (int) children.size();
      for (int i = 0; i < count; ++i) {
        if (!children[i]->removeRef()) {
          delete children[i];
        }
      }
      children.clear();
    }

    void blockStatement_t::print(printer_t &pout) const {
      // Don't print { } for root statement
      if (up) {
        pout.printIndentation();
        pout << "{\n";
        pout.addIndentation();
      }

      printChildren(pout);

      if (up) {
        pout.removeIndentation();
        pout.printIndentation();
        pout << "}\n";
      }
    }

    void blockStatement_t::printChildren(printer_t &pout) const {
      const int count = (int) children.size();
      for (int i = 0; i < count; ++i) {
        children[i]->print(pout);
      }
    }
    //====================================

    //---[ Type ]-------------------------
    typeDeclStatement_t::typeDeclStatement_t(context_t &context_,
                                                           declarationType_t &declType_) :
      statement_t(context_),
      declType(declType_) {}


    statement_t& typeDeclStatement_t::clone() const {
      return *(new typeDeclStatement_t(context, declType));
    }

    int typeDeclStatement_t::type() const {
      return statementType::typeDecl;
    }

    bool typeDeclStatement_t::hasScope() const {
      return (dynamic_cast<classType*>(&declType)
              || dynamic_cast<functionType*>(&declType));
    }

    void typeDeclStatement_t::print(printer_t &pout) const {
      declType.printDeclaration(pout);
    }

    classAccessStatement_t::classAccessStatement_t(context_t &context_,
                                                   const int access_) :
      statement_t(context_),
      access(access_) {}

    statement_t& classAccessStatement_t::clone() const {
      return *(new classAccessStatement_t(context, access));
    }

    void classAccessStatement_t::print(printer_t &pout) const {
      pout.removeIndentation();

      pout.printIndentation();
      switch (access) {
      case classAccess::private_  : pout << "private:\n"  ; break;
      case classAccess::protected_: pout << "protected:\n"; break;
      case classAccess::public_   : pout << "public:\n"   ; break;
      }

      pout.addIndentation();
    }
    //====================================

    //---[ Expression ]-------------------
    expressionStatement_t::expressionStatement_t(context_t &context_) :
      statement_t(context_) {}

    statement_t& expressionStatement_t::clone() const {
      return *(new expressionStatement_t(context));
    }

    int expressionStatement_t::type() const {
      return statementType::expression;
    }

    void expressionStatement_t::print(printer_t &pout) const {
    }

    declarationStatement_t::declarationStatement_t(context_t &context_) :
      statement_t(context_) {}

    statement_t& declarationStatement_t::clone() const {
      return *(new declarationStatement_t(context));
    }

    int declarationStatement_t::type() const {
      return statementType::declaration;
    }

    void declarationStatement_t::print(printer_t &pout) const {
    }
    //====================================

    //---[ Goto ]-------------------------
    gotoStatement_t::gotoStatement_t(context_t &context_,
                                     const std::string &name_) :
      statement_t(context_),
      name(name_) {}

    statement_t& gotoStatement_t::clone() const {
      return *(new gotoStatement_t(context, name));
    }

    void gotoStatement_t::print(printer_t &pout) const {
      pout.printIndentation();
      pout << "goto " << name << ";\n";
    }

    gotoLabelStatement_t::gotoLabelStatement_t(context_t &context_,
                                               const std::string &name_) :
      statement_t(context_),
      name(name_) {}

    statement_t& gotoLabelStatement_t::clone() const {
      return *(new gotoLabelStatement_t(context, name));
    }

    void gotoLabelStatement_t::print(printer_t &pout) const {
      pout << name << ":\n";
    }
    //====================================

    //---[ Namespace ]--------------------
    namespaceStatement_t::namespaceStatement_t(context_t &context_,
                                               const std::string &name_) :
      blockStatement_t(context_),
      name(name_) {}

    statement_t& namespaceStatement_t::clone() const {
      return *(new namespaceStatement_t(context, name));
    }

    bool namespaceStatement_t::hasScope() const {
      return true;
    }

    void namespaceStatement_t::print(printer_t &pout) const {
      pout.printIndentation();
      pout << "namespace " << name << " {\n";

      pout.pushInlined(false);
      pout.addIndentation();
      printChildren(pout);
      pout.removeIndentation();
      pout.popInlined();

      pout.printIndentation();
      pout << "}\n";
    }
    //====================================

    //---[ While ]------------------------
    whileStatement_t::whileStatement_t(context_t &context_,
                                       statement_t &check_) :
      blockStatement_t(context_),
      check(check_) {}

    statement_t& whileStatement_t::clone() const {
      return *(new whileStatement_t(context, check.clone()));
    }

    int whileStatement_t::type() const {
      return statementType::while_;
    }

    bool whileStatement_t::hasScope() const {
      return true;
    }

    void whileStatement_t::print(printer_t &pout) const {
      pout.printStartIndentation();
      pout << "while (";
      pout.pushInlined(true);
      check.print(pout);
      pout.popInlined();
      pout << ") {\n";

      pout.pushInlined(false);
      pout.addIndentation();
      printChildren(pout);
      pout.removeIndentation();
      pout.popInlined();

      pout.printIndentation();
      pout << '}';
      pout.printEndNewline();
    }
    //====================================

    //---[ For ]--------------------------
    forStatement_t::forStatement_t(context_t &context_,
                                   statement_t &init_,
                                   statement_t &check_,
                                   statement_t &update_) :
      blockStatement_t(context_),
      init(init_),
      check(check_),
      update(update_) {}

    statement_t& forStatement_t::clone() const {
      return *(new forStatement_t(context,
                                  init.clone(),
                                  check.clone(),
                                  update.clone()));
    }

    int forStatement_t::type() const {
      return statementType::for_;
    }

    bool forStatement_t::hasScope() const {
      return true;
    }

    void forStatement_t::print(printer_t &pout) const {
      pout.printStartIndentation();
      pout << "for (";
      pout.pushInlined(true);
      init.print(pout);
      check.print(pout);
      update.print(pout);
      pout.popInlined();
      pout << ") {\n";

      pout.pushInlined(false);
      pout.addIndentation();
      printChildren(pout);
      pout.removeIndentation();
      pout.popInlined();

      pout.printIndentation();
      pout << '}';
      pout.printEndNewline();
    }
    //====================================

    //---[ Switch ]-----------------------
    switchStatement_t::switchStatement_t(context_t &context_,
                                         statement_t &value_) :
      blockStatement_t(context_),
      value(value_) {}

    statement_t& switchStatement_t::clone() const {
      return *(new switchStatement_t(context, value.clone()));
    }

    int switchStatement_t::type() const {
      return statementType::switch_;
    }

    bool switchStatement_t::hasScope() const {
      return true;
    }

    void switchStatement_t::print(printer_t &pout) const {
      pout.printStartIndentation();
      pout << "switch (";
      pout.pushInlined(true);
      value.print(pout);
      pout.popInlined();
      pout << ") {\n";

      pout.pushInlined(false);
      pout.addIndentation();
      printChildren(pout);
      pout.removeIndentation();
      pout.popInlined();

      pout.printIndentation();
      pout << '}';
      pout.printEndNewline();
    }
    //====================================

    //---[ Case ]-------------------------
    caseStatement_t::caseStatement_t(context_t &context_,
                                     statement_t &value_) :
      statement_t(context_),
      value(value_) {}

    statement_t& caseStatement_t::clone() const {
      return *(new caseStatement_t(context, value.clone()));
    }

    void caseStatement_t::print(printer_t &pout) const {
      pout.removeIndentation();

      pout.printIndentation();
      pout << "case ";
      pout.pushInlined(true);
      value.print(pout);
      pout.popInlined();
      pout << ":\n";

      pout.addIndentation();
    }
    //====================================
  }
}
