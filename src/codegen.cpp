#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "../include/ast.hpp"
#include "../include/ast/Context.hpp"



std::string CodeGenExpr(Expression *expr, std::ofstream& Out, Context& ctxt) //could return a string which is the regname
{
  //std::cout<<"Here"<<expr->getValue();
  if(expr->IsNumberStmt())
  {
    std::string regname = ctxt.findFreeReg(Out);
    Out<<"addiu " + regname + ", " + "$zero" + ", " << (expr->getValue()>>16) <<std::endl;
    Out<<"sll " + regname + ", " + regname + ", 16" << std::endl;
    Out<<"addiu " + regname + ", " + "$zero" + ", " << ((expr->getValue()<<16)>>16) <<std::endl;
    return regname;
  }
  else if(expr->IsFloatStmt())
  {
    std::string regname = ctxt.findFreeFReg(Out);
    Out << "li.s " + regname + ", " << expr->getFValue() << std::endl;
    return regname;
  }
  else if(expr->IsDoubleStmt())
  {
    std::string regname = ctxt.findFreeFReg(Out);
    Out << "li.d " + regname + ", " << expr->getDValue() << std::endl;
    return regname;

  }
  else if(expr->IsFakeVariableExpr())
  {
    //need to load it into some register
    //find a free register

    std::string regname = ctxt.loadVar(expr->getId(), Out);

    //std::string dest = ctxt.findFreeReg(Out);
    Out << "nop"<< std::endl;
    return regname; //change this!!!

    //find the variable in
  }
  else if(expr->IsFunctionCallExpr())
  {
    //need to save the argument registers a0-a3
    //save registers
    //need to save the return address later
    std::string dest = ctxt.findFreeReg(Out);
    if(expr->getParams())
    {
    for(int i=0; i<4 && i<expr->getParams()->size(); i++) //need to calculate them + need to store them in $a0-$a1
    {
      dest = CodeGenExpr((*expr->getParams())[i], Out, ctxt);
      Out << "addiu $a" << i << ", " + dest + ", 0" << std::endl;
    }
    for(int i = 4; i<expr->getParams()->size();i++)
    {
      dest = CodeGenExpr((*expr->getParams())[i], Out, ctxt);
      Out << "sw " + dest + ", " <<  i*4 <<"($sp)" << std::endl;
    }
  }
  //ctxt.emptyReg(dest);
  //ctxt.storeregs(false, ctxt.FirstEmptyIndex()*4, Out);
    ctxt.storeregs(false, (8+4+1+(4+1)%2)*4, Out); //params!!
    Out << "jal " + expr->getName() << std::endl;
    //ctxt.reloadregs(false, ctxt.FirstEmptyIndex()*4, Out);
    ctxt.reloadregs(false, (8+4+1+(4+1)%2)*4, Out); //+params!!!
    Out << "addiu $v0, $v0, 0" << std::endl; //nop after jump
    Out << "addiu " + dest + ", $v0, 0" << std::endl;
    ctxt.emptyReg("$v0");
    return dest;
  }
  else if(expr->IsOperatorExpr())
  {
    if(!expr->IsIndexingOperator())
    {
      if(expr->getType(ctxt.getVariables())==IntType)
      {
    //call some other function
      std::string left = CodeGenExpr(expr->getLeft(), Out, ctxt);
      std::string right = CodeGenExpr(expr->getRight(), Out, ctxt);
      std::string dest = ctxt.findFreeReg(Out);
      opcode_to_code(dest, left, right, expr->getOpcode(), Out);
      ctxt.emptyReg(left);
      ctxt.emptyReg(right);
      return dest;
      }
      else if(expr->getType(ctxt.getVariables())==FloatType)
      {
      std::string left = CodeGenExpr(expr->getLeft(), Out, ctxt);
      std::string right = CodeGenExpr(expr->getRight(), Out, ctxt);
      std::string dest = ctxt.findFreeFReg(Out);
      opcode_to_code_float(dest, left, right, expr->getOpcode(), Out, FloatType);

      ctxt.emptyFReg(left);
      ctxt.emptyFReg(right);
      return dest;
    }
    else if(expr->getType(ctxt.getVariables())==DoubleType)
    {
      std::string left = CodeGenExpr(expr->getLeft(), Out, ctxt);
      std::string right = CodeGenExpr(expr->getRight(), Out, ctxt);
      std::string dest = ctxt.findFreeFReg(Out);
      opcode_to_code_float(dest, left, right, expr->getOpcode(), Out, DoubleType);
      ctxt.emptyFReg(left);
      ctxt.emptyFReg(right);
      return dest;
    }
    }
    else
    { //this is just for local vars
      std::string dest = ctxt.findFreeReg(Out);
      std::string address = ctxt.findFreeReg(Out);
      ctxt.loadIndex(expr->getLeft()->getId(), address, Out); //dest has the address of the first element of the array
      std::string right = CodeGenExpr(expr->getRight(), Out, ctxt); //right has the index
      std::string size = ctxt.findFreeReg(Out);
      Out << "addiu " + size + ", $zero, 4" << std::endl; //different if not int!!
      Out << "mult " + right + ", " + size << std::endl;
      Out << "mflo " + right << std::endl;
      Out << "add " + address + ", " + address + ", " + right << std::endl; //address of the element we need
      Out << "lw " + dest + ", 0(" + address + ")" << std::endl;
      Out << "nop" << std::endl;
      ctxt.emptyReg(address);
      ctxt.emptyReg(right);
      ctxt.emptyReg(size);
      return dest;
    }
  }
  else if(expr->IsUnary())
  {
    if(expr->getType(ctxt.getVariables())==IntType) //or char
    {
    std::string src = CodeGenExpr(expr->getExpr(), Out, ctxt);
    std::string dest = ctxt.findFreeReg(Out);
    opcode_to_code(dest, "$zero", src, expr->getOpcode(), Out); //need to fix the function! or would it work?
    if(expr->getOpcode()=="++" || expr->getOpcode() == "--" || expr->getOpcode()=="++post" || expr->getOpcode()=="--post")
    {
      ctxt.saveNewVar(src, expr->getExpr()->getId(), Out);
    }
    ctxt.emptyReg(src);
    return dest;
    }
    else
    {
      std::string src = CodeGenExpr(expr->getExpr(), Out, ctxt);
      std::string dest = ctxt.findFreeFReg(Out);
      opcode_to_code_float(dest, "$zero", src, expr->getOpcode(), Out, expr->getType(ctxt.getVariables())); //need to fix the function! or would it work?
      if(expr->getOpcode()=="++" || expr->getOpcode() == "--" || expr->getOpcode()=="++post" || expr->getOpcode()=="--post")
      {
        ctxt.saveNewVar(src, expr->getExpr()->getId(), Out, expr->getType(ctxt.getVariables()));
      }
      ctxt.emptyFReg(src);
      return dest;
    }
  }
  else if(expr->IsAssignExpr())
  {
    if(!expr->getLhs()->IsIndexingOperator())
    {
      if(expr->getType(ctxt.getVariables())==IntType)
      {
      std::string src = CodeGenExpr(expr->getRhs(), Out, ctxt);
      std::string dest = CodeGenExpr(expr->getLhs(), Out, ctxt);
      assignment_to_code(dest, src, expr->getOpcode(), Out);
      ctxt.saveNewVar(dest, expr->getLhs()->getId(), Out);
      ctxt.emptyReg(src);

    //ctxt.emptyRegifExpr(src, Out);
      return dest;
      }
      else
      {
        std::string src = CodeGenExpr(expr->getRhs(), Out, ctxt);
        std::string dest = CodeGenExpr(expr->getLhs(), Out, ctxt);
        assignment_to_code_float(dest, src, expr->getOpcode(), Out, expr->getType(ctxt.getVariables()));
        ctxt.saveNewVar(dest, expr->getLhs()->getId(), Out, expr->getType(ctxt.getVariables()));
        ctxt.emptyFReg(src);
      //ctxt.emptyRegifExpr(src, Out);
        return dest;
      }
    }
    else
    {

      std::string src = CodeGenExpr(expr->getRhs(), Out, ctxt);

      std::string dest = ctxt.findFreeReg(Out);
      std::string address = ctxt.findFreeReg(Out);
      //std::cerr<<expr->getLhs()->getLeft()->getId();
      std::string idxReg = CodeGenExpr(expr->getLhs()->getRight(), Out, ctxt);
      ctxt.loadIndex(expr->getLhs()->getLeft()->getId(), address, Out); //absolute address of the first element
      std::string size = ctxt.findFreeReg(Out);
      Out << "addiu " + size + ", $zero, 4" << std::endl; //different if not int!!
      Out << "mult " + idxReg + ", " + size << std::endl;
      Out << "mflo " + idxReg << std::endl;
      Out << "add " + address + ", " + address + ", " + idxReg << std::endl; //address of element we need
      //Out << "sw " + src + ", 0(" + dest + ")" << std::endl;
      Out << "lw " + dest + ", 0(" + address + ")"<< std::endl;
      assignment_to_code(dest, src, expr->getOpcode(), Out);
      Out << "sw " + dest + ", 0(" + address + ")" << std::endl;
      ctxt.emptyReg(src);
      ctxt.emptyReg(address);
      ctxt.emptyReg(size);
      return dest;
    }
  }
  else throw("Invalid expression!");
}

void CodeGen(const Statement *stmt, std::ofstream& Out, Context& variables, int memsize)
{


  if(stmt->IsExpressionStmt())
  {
      //find variable in hash table
        //fprintf(stderr, "here");
      std::string regname = CodeGenExpr((Expression*)stmt, Out, variables);
      if(((Expression*)stmt)->getType(variables.getVariables())==IntType)
        variables.emptyReg(regname);
      else
        variables.emptyFReg(regname);
  }
  else if(stmt->IsReturnStmt())
  {
    //evaluate return value
    //move that value to v0
      //fprintf(stderr, "here");

    std::string regname = CodeGenExpr((Expression*)stmt->getRetVal(), Out, variables);

    if(((Expression*)(stmt->getRetVal()))->getType(variables.getVariables())==IntType)
    {
      Out<<"addiu $v0, " << regname << ", 0" <<std::endl;
      variables.emptyReg(regname);
    }
    else
    {
      if(((Expression*)(stmt->getRetVal()))->getType(variables.getVariables())==FloatType)
        Out << "mov.s $f0, " + regname<<std::endl; //double??
      else
        Out << "mov.s $f0, " + regname<<std::endl;
      variables.emptyFReg(regname);
    }


    //if(funct->getName()!="main")
    //{
      variables.loadRetAddr(Out, 12*4);

      variables.reloadregs(true, 4*4, Out);
    //}
    //else

      //need to load s0-s7 back
      //ctxt.freeMem((funct->getSize()+21+(4/*+paramssize*/)%2)*4 + (funct->getSize()%2)*4, Out); //shouldn't matter i think ?? //FIX THIS
    //(funct->getSize()+21+(4+1/*+paramssize*/)%2) + (funct->getSize()%2)
    variables.freeMem(memsize, Out); //shouldn't matter i think ?? //FIX THIS
    Out << "jr $ra" << std::endl;
    Out << "nop" << std::endl;

  return;
  }

  else if(stmt->IsCompoundStmt())
  {
    //variables.findInMem("a"); std::cerr<<"compound"<<std::endl;
    std::vector<Statement*>* stmts= stmt->getStmts();
    std::vector<Variable_hash> gv;
    Context newCtxt(0, gv);

    newCtxt.enterScope(variables);
    //std::cerr<<"here\n";
    if(stmts)
    {
    for(int i = 0; i<stmts->size(); i++)
    {
      CodeGen((*stmts)[i], Out, newCtxt, memsize);
//std::cerr<<"here";
    }
    }

    newCtxt.leaveScope(variables, Out);

  }
  else if(stmt->IsDeclarationStmt())
  {

    variables.newVar(stmt->getVariable(), ((Declaration*)stmt)->getType(variables.getVariables()), stmt->getArraySize());



    if(stmt->getExpr()!=nullptr)
    {
    //std::string dest = variables.findFreeReg(Out);
    std::string regname = CodeGenExpr((Expression*)(stmt->getExpr()), Out, variables);
    //Out << "add " + dest + ", " + regname + ", $zero" << std::endl;

    variables.saveNewVar(regname, stmt->getVariable(), Out, ((Declaration*)stmt)->getType(variables.getVariables()));

    if(((Declaration*)stmt)->getType(variables.getVariables())==IntType) variables.emptyReg(regname);
    else variables.emptyFReg(regname);

    }
    else
    {
    //  variables.saveNewVar("$zero", stmt->getVariable(), Out);
    }



  }
  else if(stmt->IsIfElseStmt())
  {
    std::string regCond = CodeGenExpr((Expression*)stmt->getCond(), Out, variables);
    //std::string iflabel = makeName("if");
    std::string elselabel = makeName("else");
    std::string afteriflabel = makeName("afterif");
    //if cond true jump to iflabel
    //nop
    if(stmt->getElseStmts())
      Out << "beq " + regCond + ", $zero, " +  elselabel << std::endl;
    else
      Out << "beq " +  regCond + ", $zero, " + afteriflabel<< std::endl;
    variables.emptyReg(regCond);
    //if there is an else jump to elselabel
    Out << "addiu $v0, $v0, 0" << std::endl; //nop
    CodeGen(stmt->getIfStmts(), Out, variables, memsize);
    Out << "j " + afteriflabel << std::endl;
    Out << "addiu $v0, $v0, 0" << std::endl;
    //jump to afterifelse
    if(stmt->getElseStmts())
    {
    Out << elselabel +":" << std::endl;
    //if(stmt->getElseStmts()!=nullptr)
      CodeGen(stmt->getElseStmts(), Out, variables, memsize);
    Out << "j " + afteriflabel << std::endl;
    Out << "addiu $v0, $v0, 0" << std::endl;
    }

    //jump to afterifelse
    Out << afteriflabel + ":" <<std::endl;



  }
  else if(stmt->IsWhile())
  {
    std::string whilelabel = makeName("while");
    std::string afterwhilelabel = makeName("afterwhile");
    Out << whilelabel + ":" << std::endl;
    std::string regCond = CodeGenExpr((Expression*)stmt->getCond(), Out, variables);
    Out << "beq " + regCond + ", $zero, " +  afterwhilelabel << std::endl;
    Out << "addiu $v0, $v0, 0" << std::endl;
    variables.emptyReg(regCond);
    CodeGen(stmt->getCompoundStmt(), Out, variables, memsize);
    Out << "j " + whilelabel <<std::endl;
    Out<< "addiu $zero, $zero, 0" << std::endl;
    Out << afterwhilelabel + ":" <<std::endl;
  }
  else if(stmt->IsFor())
  {
    std::string forlabel = makeName("for");
    std::string afterforlabel = makeName("afterfor");
    if(stmt->getFirst())
      CodeGen(stmt->getFirst(), Out, variables, memsize);
    Out << forlabel + ":" << std::endl;
    std::string regCond = CodeGenExpr((Expression*)stmt->getSecond(), Out, variables);
    Out << "beq " + regCond + ", $zero, " +  afterforlabel << std::endl;
    Out << "addiu $v0, $v0, 0" << std::endl;
    variables.emptyReg(regCond);
    CodeGen(stmt->getCompoundStmt(), Out, variables, memsize);
    if(stmt->getThird())
      CodeGenExpr((Expression*)stmt->getThird(), Out, variables);
    Out << "j " + forlabel <<std::endl;
    Out<< "addiu $zero, $zero, 0" << std::endl;
    Out << afterforlabel + ":" <<std::endl;
  }
  else throw("Invalid statement!");
}

void CompileFunct(const Function *funct, std::ofstream& Out, std::vector<Variable_hash> global_vars)
{
  //label:
  Out << funct->getName() + ":" << std::endl;

  CompoundStmt *body = funct->getBody();
  int ParamSize = funct->getParams()?funct->getParams()->size():0; //should be different with different types
  Context ctxt((funct->getSize()+21+(4+1+ParamSize)%2 + (funct->getSize()%2)), global_vars);
  //need to save return address
  //need to save registers
  //fprintf(stderr, c_str(std::to_string(funct->getSize())));
  //std::cerr<<std::to_string(funct->getSize());
//ctxt.printStack();
    int memsize = (funct->getSize()+21+(4+1+ParamSize)%2) + (funct->getSize()%2);
  Out << "addiu $t1, $sp, 0" << std::endl; //store previous stack pointer
ctxt.allocateMem((funct->getSize()+21+(4+1+ParamSize)%2) + (funct->getSize()%2), Out);
int returnAddr;
if(ParamSize<=4) returnAddr = 8 + 4;
else returnAddr = 8 + ParamSize;
if(funct->getParams())
  for(int i = 4; i<(funct->getParams())->size(); i++)
  {
    Out << "lw $t0, " << i*4 << "($t1)" << std::endl;
    Out << "nop" << std::endl;
    Out << "sw $t0, " << (returnAddr + returnAddr%2 + i-4 + 1)*4 << "($sp)" << std::endl;
  }
//std::cerr<<(funct->getSize()+21+(4+1+ParamSize)%2) + (funct->getSize()%2)<<std::endl;
  if(funct->getParams())
  {
  for(int i = 0; i<(funct->getParams())->size(); i++)
  {
    CodeGen((*funct->getParams())[i], Out, ctxt, memsize);
  }
  for(int i = 0; i<4 && i< ParamSize; i++)
  {
    Out << "sw $a" << i << ", " << i*4 << "($sp)" << std::endl;
  }
  int nrOffloats = 0;
  int stackidx = 0;
  for(int i = 0; i<4 && nrOffloats<2 && i<(funct->getParams())->size(); i++)
  {
    std::string prec;
    if(((Declaration*)((*funct->getParams())[i]))->getType(ctxt.getVariables())==FloatType)
    {

      prec = "s";
      //stackidx++;
    }
     else if(((Declaration*)((*funct->getParams())[i]))->getType(ctxt.getVariables())==DoubleType)
    {
      prec = "d";
    }
    if(((Declaration*)((*funct->getParams())[i]))->getType(ctxt.getVariables())==FloatType || ((Declaration*)(*funct->getParams())[i])->getType(ctxt.getVariables())==DoubleType)
    {
    Out << "s." + prec + " $f" <<12 + nrOffloats*2 << ", " << stackidx*4 << "($sp)" << std::endl;

    }

    if(((Declaration*)((*funct->getParams())[i]))->getType(ctxt.getVariables())==FloatType)
    {
      stackidx++;
      nrOffloats++;
    }
     else if(((Declaration*)((*funct->getParams())[i]))->getType(ctxt.getVariables())==DoubleType)
    {
      stackidx += 2;
      nrOffloats++;
    }
    else if(((Declaration*)((*funct->getParams())[i]))->getType(ctxt.getVariables())==IntType)
   {
     stackidx++;
   }


  }
  }
  ctxt.setMemEmpty(returnAddr+returnAddr%2);


  //ctxt.allocateMem((funct->getSize()+21+(4+1+ParamSize)%2) + (funct->getSize()%2), Out); //FIX THIS

  if(funct->getName()=="main")
  {
    //ctxt.allocateMem((funct->getSize()+21+(4+1/*+paramssize*/)%2)*4 + (funct->getSize()%2)*4, Out); //FIX THIS
  }
  else
  {

    ctxt.saveRetAddr(Out, returnAddr*4); //+params size

    //need to save s0-s7
    ctxt.storeregs(true, 4*4, Out); //+params size
    //std::cerr<<"here";
    //ctxt.printStack();

  }
  //for loop for the parameters maybe?

  CodeGen(body, Out, ctxt, memsize);

   //is this correct?
  /*if(funct->getName()!="main")
  {
    ctxt.loadRetAddr(Out, 12*4);

    ctxt.reloadregs(true, 4*4, Out);
  }
  else
  {*/
    //need to load s0-s7 back
    //ctxt.freeMem((funct->getSize()+21+(4/*+paramssize*/)%2)*4 + (funct->getSize()%2)*4, Out); //shouldn't matter i think ?? //FIX THIS
  //}
/*ctxt.freeMem((funct->getSize()+21+(4+1/*+paramssize*///)%2) + (funct->getSize()%2), Out); //shouldn't matter i think ?? //FIX THIS
  //Out<<"jr $ra" <<std::endl;
}
