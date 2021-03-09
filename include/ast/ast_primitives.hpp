#ifndef ast_primitives_hpp
#define ast_primitives_hpp

#include <string>
#include <iostream>
#include "ast_statements.hpp"

class Variable
    : public Expression
{
private:
    std::string id;
public:
    Variable(const std::string &_id)
        : id(_id)
    {}

    const std::string getId() const
    { return id; }

    virtual void print(std::ostream &dst) const override
    {
        dst<<id;
    }

    virtual bool IsVariableStmt() const override {return true;}
    /*virtual double evaluate(
        const std::map<std::string,double> &bindings
    ) const override
    {
        // TODO-B : Run bin/eval_expr with a variable binding to make sure you understand how this works.
        // If the binding does not exist, this will throw an error
        return bindings.at(id);
    }*/
};

class Number
    : public Expression
{
private:
    int value;
public:
    Number(int _value)
        : value(_value)
    {}
      virtual int getValue() const override {return value;}

      virtual bool IsNumberStmt() const override {return true;}
    virtual void print(std::ostream &dst) const override
    {
        dst<<value;
    }

    /*virtual double evaluate(
        const std::map<std::string,double> &bindings
    ) const override
    {
        // TODO-A : Run bin/eval_expr with a numeric expression to make sure you understand how this works.
        return value;
    }*/
};


#endif
