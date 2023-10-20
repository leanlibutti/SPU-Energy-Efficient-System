#include <LCDItem.h>

void printLCD(char text[], int x, int y, LiquidCrystal_I2C lcd){
  int pos = 0, line = y, col = x;
  while((text[pos] != '\0') && (line < 4)){
    if((col == MAX_LINE_WIDTH) || (text[pos] == '\n')) {
      col = 0; line++;
      if(text[pos] == '\n') pos++;
    }
    lcd.setCursor(col, line);
    if(text[pos] == '\r')
      lcd.write(0);
    else
      lcd.write(text[pos]);
    col++;
    pos++;
  }
}

LCDItem::LCDItem(char* t, int i):
  lcd(LiquidCrystal_I2C(0x3F, 20, 4)),
  staticItem(false),
  id(i),
  hasUpdateFunction(false),
  hasInitFunction(false),
  disposableItem(false){
  itemTitle = new char[MAX_LINE_WIDTH];
  strncpy(itemTitle, t, MAX_LINE_WIDTH);
};

LCDItem::LCDItem(char* t, char* (*u)(int), int i): LCDItem(t, i){
  hasUpdateFunction = true;
  onUpdate = u;
}

LCDItem::LCDItem(char* t, void (*in)(int), int i): LCDItem(t, i){
  hasInitFunction = true;
  initialize = in;
}

LCDItem::LCDItem(char* t, void (*in)(int), char* (*u)(int), int i): LCDItem(t, i){
  hasUpdateFunction = true;
  onUpdate = u;
  hasInitFunction = true;
  initialize = in;
}

char* LCDItem::title(){
  return itemTitle;
}

void LCDItem::render(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Item vacio");
}

void LCDItem::moveUp(){}

void LCDItem::moveDown(){}

bool LCDItem::isStatic(){
  return staticItem;
}

void LCDItem::makeStatic(){
  staticItem = true;
}

bool LCDItem::isDisposable(){
  return disposableItem;
}

void LCDItem::makeDisposable(){
  disposableItem = true;
}

bool LCDItem::select(){
  return true;
}

void LCDItem::setScreen(LiquidCrystal_I2C& l){
  lcd = l;
}

bool LCDItem::goBack(){
  return false;
}

void LCDItem::setTitle(char* t){
  strncpy(itemTitle, t, MAX_LINE_WIDTH);
}

void LCDItem::update(){
  if(hasUpdateFunction){
    onUpdate(id);
  }
}

void LCDItem::init(){
  if(hasInitFunction) initialize(id);
}

LCDItem::~LCDItem(){
  delete itemTitle;
}

LCDSubmenu::LCDSubmenu(char* t, char* m, int i): LCDSubmenu(t, i){
  strncpy(emptyMessage, m, MAX_LINE_WIDTH);
}

LCDSubmenu::~LCDSubmenu(){
  empty();
}

void LCDSubmenu::init(){
  LCDItem::init();
  pointer = 0;
}

void LCDSubmenu::update(){
  if(isHome){
    if(hasUpdateFunction) onUpdate(id);
  } else
    current->update();
}

void LCDSubmenu::addItem(LCDItem* item){
  if(itemAmount < MAX_ITEM_NUMBER){
    items[itemAmount++]= item;
    items[itemAmount-1]->setScreen(lcd);
    item->setParent(this);
  }
}

void LCDSubmenu::render(){
  if(isHome){
    renderHome();
  } else {
    current->render();
  }
}

void LCDSubmenu::renderHome(){
  lcd.clear();
  if(pointer != 0){
    lcd.setCursor(1, 0);
    lcd.print(items[pointer-1]->title());
  } else {
    lcd.setCursor(0, 0);
    lcd.print(itemTitle);
    if(itemAmount == 0){
      if(emptyMessage != nullptr)
        printLCD(emptyMessage, 1, 1, lcd);
      return;
    }
  }
  for(int i = 0; (pointer+i < itemAmount) && (i < 3); i++){
    lcd.setCursor(1, i+1);
    lcd.print(items[pointer+i]->title());
  }
  lcd.setCursor(0, 1);
  lcd.print(">");
}

void LCDSubmenu::moveUp(){
  if(isHome){
    pointer++;
    if(pointer > itemAmount - 1) pointer=itemAmount - 1;
    if(itemAmount == 0) pointer = 0;
  } else {
    current->moveUp();
  }
}

void LCDSubmenu::moveDown(){
  if(isHome){
    pointer--;
    if(pointer < 0) pointer=0;
  } else {
    current->moveDown();
  }
}

bool LCDSubmenu::select(){
  if(isHome){
    if(itemAmount == 0) return true;
    current = items[pointer];
    isHome = false;
    current->init();
  } else {
    bool endExec = current->select();
    if(endExec) {
      isHome= true;
      init();
    }
  }
  return false;
}

void LCDSubmenu::setScreen(LiquidCrystal_I2C& l){
  LCDItem::setScreen(l);
  for(int i = 0; i < itemAmount; i++){
    items[i]->setScreen(l);
  }
}

void LCDSubmenu::goHome(){
  current->goHome();
  isHome = true;
  init();
}

bool LCDSubmenu::goBack(){
  pointer = 0;
  if(!isHome){
    if(!current->goBack()) {
      isHome = true;
      init();
    }
    return true;
  } else {
    for(int i = 0; i < itemAmount; i++){
      if(items[i]->isDisposable()){
        delete items[i];
        for(int j= i; j < itemAmount-1; j++){
          items[j] = items[j+1];
        }
        itemAmount--;
      }
    }
    return false;
  }
}

void LCDSubmenu::empty(){
  for(int i = 0; i < itemAmount; i++){
    if(!items[i]->isStatic()) {
      delete items[i];
    }
  }
  itemAmount = 0;
}

void LCDFunction::init(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(itemTitle);
  lcd.setCursor(0, 1);
  lcd.print(waitMsg);
  resultMsg = action(id);
}

void LCDFunction::render(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(itemTitle);
  lcd.setCursor(0, 1);
  printLCD(resultMsg, 0, 1, lcd);
}

bool LCDFunction::select(){
  //parent->goHome();
  return true;
}

void LCDFunction::update(){
  if(hasUpdateFunction){
    resultMsg = onUpdate(id);
  }
}

LCDForm::LCDForm(char* t, char* (*f)(int argc, String argv[], int), void (*i)(int), int id):
  LCDForm(t, f, id)
{
  hasInitFunction = true;
  initialize = i;
}

void LCDForm::init(){
  pointer = 0;
  selected = false;
  submitted = false;
  for(int i = 0; i < fieldAmount; i++){
    fields[i].intValue = fields[i].defaultValue;
    fields[i].strValue[0] = '\0';
  }
  if(hasInitFunction) initialize(id);
}

void LCDForm::render(){
  lcd.clear();
  if(!submitted){
    if(pointer != 0){
      lcd.setCursor(1, 0);
      printField(pointer-1);
    } else {
      lcd.setCursor(0, 0);
      lcd.print(itemTitle);
    }
    int i;
    for(i = 0; (i < 3) && (pointer+i < fieldAmount); i++){
      lcd.setCursor(1, i+1);
      if(selected && i == 0) lcd.setCursor(2, 1);
      printField(pointer+i);
    }
    if(i <= 2){
      lcd.setCursor(1, i+1);
      lcd.print("Enviar");
    }
    lcd.setCursor(0, 1);
    lcd.print(">");
    if(selected && (fields[pointer].type == TYPE_STRING)){
      lcd.setCursor(strPointer+strlen(fields[pointer].name)+4, 1);
      if(charSelected){
        lcd.blink();
      } else {
        lcd.noBlink();
        lcd.cursor();
      }
    } else {
      lcd.noBlink();
      lcd.noCursor();
    }
  } else {
    lcd.setCursor(0, 0);
    lcd.print(itemTitle);
    lcd.setCursor(1, 1);
    lcd.print(result);
  }
}

void LCDForm::printField(int i){
  lcd.print(fields[i].name);
  lcd.print(": ");
  switch(fields[i].type){
    case TYPE_STRING:{
          char str[MAX_FIELD_LENGTH];
          int pos = 0;
          while(fields[i].strValue[pos] && (pos < MAX_FIELD_LENGTH)){
            str[pos] = fields[i].strValue[pos];
            pos++;
          }
          if(pos < MAX_FIELD_LENGTH) str[pos] = '~';
          lcd.print(str);
          break;}
    case TYPE_INT:
      lcd.print(fields[i].intValue);
      break;
  }
}

void LCDForm::addField(char* name, int type){
  addField(name, type, 0, TYPE_UNDEFINED, TYPE_UNDEFINED);
}

void LCDForm::addField(char* name, int type, int defValue){
  addField(name, type, defValue, TYPE_UNDEFINED, TYPE_UNDEFINED);
}

void LCDForm::addField(char* name, int type, int defValue, int min, int max){
  if(fieldAmount < MAX_FIELD_NUMBER){
    fields[fieldAmount].name = name;
    fields[fieldAmount].type = type;
    fields[fieldAmount].strValue[0] = '\0';
    fields[fieldAmount].intValue = 0;
    fields[fieldAmount].defaultValue = defValue;
    fields[fieldAmount].maxValue = max;
    fields[fieldAmount].minValue = min;
    fieldAmount++;
  }
}

void LCDForm::setField(char* name, int value){
  for(int i = 0; i < fieldAmount; i++){
    if(strcmp(fields[i].name, name) == 0){
      fields[i].intValue = value;
      return;
    }
  }
}

void LCDForm::setField(const char* name,const  char* value){
  for(int i = 0; i < fieldAmount; i++){
    if(strcmp(fields[i].name, name) == 0){
      strncpy(fields[i].strValue, value, 20);
      fields[i].strValue[19] = '\0';
      return;
    }
  }
}

void LCDForm::moveUp(){
  if(selected){
    increaseValue(pointer);
  } else {
    pointer++;
    if(pointer == fieldAmount+1) pointer--;
  }
}

void LCDForm::moveDown(){
  if(selected){
    decreaseValue(pointer);
  } else {
    pointer--;
    if(pointer < 0) pointer = 0;
  }
}

void LCDForm::increaseValue(int i){
  field* f = &fields[i];
  /*switch(f->type){
    case TYPE_STRING:
      if(charSelected){
        f->strValue[strPointer]++;
        switch(f->strValue[strPointer]){
          case 1: f->strValue[strPointer] = 65; break;
          case 91: f->strValue[strPointer] = 97; break;
          case 123: f->strValue[strPointer] = 48; break;
          case 58: f->strValue[strPointer] = 57; break;
        }
      } else {
        if(f->strValue[strPointer] != '\0')
          strPointer++;
      }
      break;
    case TYPE_INT:
      if((f->maxValue == TYPE_UNDEFINED) || (f->intValue < f->maxValue))
        f->intValue++;
      break;
  }*/
  switch(f->type){
    case TYPE_STRING:
      if(charSelected){
        f->strValue[strPointer]++;
        switch(f->strValue[strPointer]){
          case 1: f->strValue[strPointer] = 32; break;
          case 126: f->strValue[strPointer] = 0; break;
        }
      } else {
        if(f->strValue[strPointer] != '\0')
          strPointer++;
      }
      break;
    case TYPE_INT:
      if((f->maxValue == TYPE_UNDEFINED) || (f->intValue < f->maxValue))
        f->intValue++;
      break;
  }
}

void LCDForm::decreaseValue(int i){
  field* f = &fields[i];
  /*switch(f->type){
    case TYPE_STRING:
      if(charSelected){
        f->strValue[strPointer]--;
        switch(f->strValue[strPointer]){
          case 64: f->strValue[strPointer] = 0; break;
          case 96: f->strValue[strPointer] = 90; break;
          case 47: f->strValue[strPointer] = 122; break;
          case -1: f->strValue[strPointer] = 0; break;
        }
      } else {
        if(strPointer != 0)
          strPointer--;
      }
      break;
    case TYPE_INT:
      if((f->minValue == TYPE_UNDEFINED) || (f->intValue > f->minValue))
        f->intValue--;
      break;
  }*/
  switch(f->type){
    case TYPE_STRING:
      if(charSelected){
        f->strValue[strPointer]--;
        switch(f->strValue[strPointer]){
          case 31: f->strValue[strPointer] = 0; break;
          case -1: f->strValue[strPointer] = 125; break;
        }
      } else {
        if(strPointer != 0)
          strPointer--;
      }
      break;
    case TYPE_INT:
      if((f->minValue == TYPE_UNDEFINED) || (f->intValue > f->minValue))
        f->intValue--;
      break;
  }
}

bool LCDForm::select(){
  if(submitted){
    return true;
  } else if(pointer == fieldAmount){
    submit();
  } else if(!selected) selected = true;
  else if(fields[pointer].type == TYPE_STRING){
    charSelected = !charSelected;
  }
  return false;
}

bool LCDForm::goBack(){
  if(selected){
    selected = false;
    charSelected = false;
    strPointer = 0;
    return true;
  } else {
    return false;
  }
}

void LCDForm::submit(){
  String results[MAX_FIELD_NUMBER];
  int n = 0;
  for(int i = 0; i < fieldAmount; i++){
    switch(fields[i].type){
      case TYPE_STRING:
        results[i] = String(fields[i].strValue);
        break;
      case TYPE_INT:
        results[i] = String(fields[i].intValue);
        break;
    }
    n++;
  }
  result = callback(n, results, id);
  submitted = true;
}

void LCDForm::copyIn(LCDForm* form){
  for(int i = 0; i < fieldAmount; i++){
    form->addField(fields[i].name, fields[i].type, fields[i].defaultValue,
      fields[i].minValue, fields[i].maxValue);
  }
}

LCDForm* LCDForm::copy(int newId){
  LCDForm* form;
  if(hasInitFunction)
    form = new LCDForm(itemTitle, callback, initialize, (newId == UNDEFINED_ID)?id:newId);
  else
    form = new LCDForm(itemTitle, callback, (newId == UNDEFINED_ID)?id:newId);
  copyIn(form);
  return form;
}

void LCDText::init(){
  pointer = 0;
  if(hasInitFunction) initialize(id);
}

void LCDText::render(){
  int pos = 0, column = 0, line = 0;
  lcd.clear();
  if(pointer == 0){
    lcd.setCursor(0, 0);
    lcd.print(itemTitle);
    line++;
  } 
  for(int i = 0; i < pointer; i++){
    while((body[pos] != '\n') && (body[pos] != '\0') && (column < MAX_LINE_WIDTH)) {
      pos++;
      column++;
    }
    if(body[pos] == '\n') pos++;
    column = 0;
  }
  while(body[pos] != '\0'){
    if((body[pos] == '\n') || (column == MAX_LINE_WIDTH)){
      column = 0;
      line++;
      if(body[pos] == '\n') pos++;
      if(line == 4) break;
    }
    lcd.setCursor(column, line);
    lcd.print(body[pos]);
    pos++;
    column++;
  }
}

void LCDText::moveUp(){
  pointer++;
}

void LCDText::moveDown(){
  pointer--;
  if(pointer < 0) pointer = 0;
}
