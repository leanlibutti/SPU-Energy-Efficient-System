#include <LCDMenu.h>
#include <Map.h>

LCDItem* emptyItem = new LCDItem("");

LCDMenu::LCDMenu(const LiquidCrystal_I2C& l) : 
  lcd(l),
  current(emptyItem)
{
  lcd.init();
  lcd.init();
  Wire.begin(D2, D1);
  lcd.backlight();
  lcd.setCursor(0, 0);
  render();
}

void LCDMenu::render(){
  current->render();
}

void LCDMenu::moveUp(){
  current->moveUp();
  render();
}

void LCDMenu::moveDown(){
  current->moveDown();
  render();
}

void LCDMenu::select(){
  current->select();
  render();
}

void LCDMenu::addHome(LCDItem* item){
  current = item;
  current->setScreen(lcd);
  render();
}

void LCDMenu::goHome(){
  current->goHome();
  render();
}

void LCDMenu::goBack(){
  current->goBack();
  render();
}

void LCDMenu::update(){
  current->update();
  render();
}