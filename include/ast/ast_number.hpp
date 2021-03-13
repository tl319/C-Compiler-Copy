#ifndef ast_primitives_hpp
#define ast_primitives_hpp

#include <string>
#include <iostream>


/*class Variable
    : public Expression
{
private:
    std::string id;
    VarType type;
public:
    Variable(const std::string &_id, VarType type)
        : id(_id), type(type)
    {}

    const std::string getId() const
    { return id; }

    VarType getType() const {return type;}

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
    }
};*/

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
};


#endif