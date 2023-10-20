#ifndef LCDITEM_H
#define LCDITEM_H

#define MAX_ITEM_NUMBER 20
#define MAX_FIELD_NUMBER 20
#define MAX_FIELD_LENGTH 20
#define MAX_LINE_WIDTH 20
#define TYPE_UNDEFINED -1
#define TYPE_STRING 0
#define TYPE_INT 1
#define UNDEFINED_ID -1

#include <Arduino.h>
#include <string.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

class LCDItem {
  public:
    LCDItem(char* t, int i = UNDEFINED_ID);
    LCDItem(char* t, char* (*u)(int), int i = UNDEFINED_ID);
    LCDItem(char* t, void (*in)(int), int i = UNDEFINED_ID);
    LCDItem(char* t, void (*in)(int), char* (*u)(int), int i = UNDEFINED_ID);
    virtual void moveUp();
    virtual void moveDown();
    virtual bool select();
    virtual void render();
    virtual void goHome(){};
    virtual bool goBack();
    virtual void init();
    virtual void setTitle(char* t);
    virtual void update();
    virtual bool isStatic();
    virtual void makeStatic();
    virtual bool isDisposable();
    virtual void makeDisposable();
    virtual void setUpdateFunction(char* (*u)(int)){ onUpdate = u; hasUpdateFunction = true;};
    virtual void setInitFunction(void (*i)(int)){ initialize = i; hasInitFunction = true; };
    virtual ~LCDItem();
    virtual void setParent(LCDItem* p){parent = p;};
    char* title();
    virtual void setScreen(LiquidCrystal_I2C& l);
  protected:
    char* itemTitle;
    LiquidCrystal_I2C lcd;
    int id;
    LCDItem* parent;
    char* (*onUpdate)(int);
    bool hasUpdateFunction;
    bool hasInitFunction;
    bool staticItem;
    bool disposableItem;
    void (*initialize)(int);
};

class LCDSubmenu: public LCDItem {
  public:
    LCDSubmenu(char* t, int i = UNDEFINED_ID) : LCDItem(t, i), itemAmount(0), pointer(0), isHome(true){emptyMessage = new char[MAX_LINE_WIDTH];};
    LCDSubmenu(char* t, char* m, int i = UNDEFINED_ID);
    LCDSubmenu(char* t, char* m, void (*in)(int), int i = UNDEFINED_ID): LCDSubmenu(t, m, i){setInitFunction(in);}
    LCDSubmenu(char* t, void (*in)(int), int i = UNDEFINED_ID): LCDSubmenu(t, i){setInitFunction(in);}
    ~LCDSubmenu();
    void moveUp();
    void moveDown();
    bool select();
    void render();
    void addItem(LCDItem* item);
    void empty();
    void setScreen(LiquidCrystal_I2C& l);
    void goHome();
    bool goBack();
    void init();
    void update();
  protected:
    mutable LCDItem* items[MAX_ITEM_NUMBER];
    int itemAmount;
    int pointer;
    char* emptyMessage;
    bool isHome;
    LCDItem* current;
    void renderHome();
};

class LCDFunction: public LCDItem {
  public:
    LCDFunction(char* t, char*(*f)(int), char* msg = "", int i = UNDEFINED_ID): LCDItem(t, i), action(f), waitMsg(msg), resultMsg("") {};
    LCDFunction(char* t, char*(*f)(int), int i): LCDFunction(t, f, "", i){};
    void render();
    void init(); 
    bool select();
    void update();
  protected:
    char* (*action)(int);
    char* waitMsg;
    char* resultMsg;
};

class LCDForm: public LCDItem {
  public:
    LCDForm(char* t, char* (*f)(int argc, String argv[], int), int i = UNDEFINED_ID): 
      LCDItem(t, i), callback(f), fieldAmount(0), charSelected(false), strPointer(0) {};
    LCDForm(char* t, char* (*f)(int argc, String argv[], int), void (*i)(int), int id = UNDEFINED_ID);
    void moveUp();
    void moveDown();
    bool select();
    void render();
    bool goBack();
    void init();
    void addField(char* n, int t);
    void addField(char* n, int t, int d);
    void addField(char* n, int t, int d, int min, int max);
    void setField(char* n, int v);
    void setField(const char* n,const char* v);
    void copyIn(LCDForm* form);
    LCDForm* copy(int id = UNDEFINED_ID);
  protected:
    typedef struct t_field{
      t_field(): type(TYPE_UNDEFINED){};
      char* name;
      int type;
      int defaultValue;
      int maxValue;
      int minValue;
      char strValue[MAX_FIELD_LENGTH];
      int intValue;
    } field;
    field fields[MAX_FIELD_NUMBER];
    int fieldAmount;
    int pointer;
    bool selected;
    int strPointer;
    bool charSelected;
    char* (*callback)(int argc, String argv[], int);
    bool submitted;
    char* result;
    void printField(int index);
    void increaseValue(int index);
    void decreaseValue(int index);
    void submit();
};

class LCDText: public LCDItem {
  public:
    LCDText(char* t, char* b, int i = UNDEFINED_ID): LCDItem(t, i), body(b), pointer(0) {};
    LCDText(char* t, char* b, void (*in)(int), int i = UNDEFINED_ID): LCDItem(t, i), body(b), pointer(0) {};
    void render();
    void moveUp();
    void moveDown();
    void init();
  protected:
    char* body;
    int pointer;
};


#endif
