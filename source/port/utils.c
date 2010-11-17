/* PituKa - Amstrad CPC Emulator
   (c) Copyright 2004-2005 David Skywalker

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "../caprice/cap32.h"
#include "../global.h"

extern byte keyboard_matrix[16];
extern byte keyboard_translation_SDL[320];
extern byte bit_values[8];

extern char spool_cad[256];
extern unsigned char spool;
extern unsigned char spool_act;
extern Pituka_SpoolKeys spoolkeys;

void process_spoolkey(int spool_ticks){
  static byte tecla_pulsada='\0';
  static int tiempo_spool = 0;
  static bool pulsada = false;

  if ( spoolkeys.comienzo == true ) {
    spoolkeys.cur = &spoolkeys.contenido[0];
    spoolkeys.cont = strlen((char *) spoolkeys.contenido) + 1; //contenido + intro final
    tecla_pulsada = '\0';
    tiempo_spool = 0;
    pulsada = false;
    //no mas iniciaciones
    spoolkeys.comienzo = false;
  }
            
  if ( (tiempo_spool+spool_ticks)>50 ) { //si han pasado 50ms
    tiempo_spool -= 50; //asi siempre empezamos de 0 :)

    tecla_pulsada=keyboard_translation_SDL[*spoolkeys.cur];

    if ( (tecla_pulsada != 0xff) ) 
    {
      if ( pulsada == false ) {
        keyboard_matrix[tecla_pulsada >> 4] &= ~bit_values[tecla_pulsada & 7]; // tecla pulsada
        pulsada = true;
      }else{
        keyboard_matrix[tecla_pulsada >> 4] |= bit_values[tecla_pulsada & 7];  // tecla soltada
        pulsada = false;
      }
     }
     else
     {
       if ( *spoolkeys.cur=='|' ) {
         if ( strcmp((char *) spoolkeys.contenido,"|cpm") == 0 ){ //son iguales :?
           if ( pulsada == false ){
             keyboard_matrix[ 0x32 >> 4] &= ~bit_values[ 0x32 & 7]; // tecla pulsada -> "|"
             keyboard_matrix[ 0x25 >> 4] &= ~bit_values[ 0x25 & 7]; // tecla pulsada -> 0x25 (SHIFT)
             pulsada = true;
           }else{
             keyboard_matrix[ 0x32 >> 4] |= bit_values[ 0x32 & 7];  // tecla soltada -> "|"
             keyboard_matrix[ 0x25 >> 4] |= bit_values[ 0x25 & 7];  // tecla soltada -> 0x25 (SHIFT)
             pulsada = false;
           }
         }else{
           if ( pulsada == false ){
             keyboard_matrix[ 0x81 >> 4] &= ~bit_values[ 0x81 & 7]; // tecla pulsada -> "
             keyboard_matrix[ 0x25 >> 4] &= ~bit_values[ 0x25 & 7]; // tecla pulsada -> 0x25 (SHIFT)
             pulsada = true;
           }else{
             keyboard_matrix[ 0x81 >> 4] |= bit_values[ 0x81 & 7];  // tecla soltada -> "
             keyboard_matrix[ 0x25 >> 4] |= bit_values[ 0x25 & 7];  // tecla soltada -> 0x25 (SHIFT)
             pulsada = false;
           }                
         }
       }else if( *spoolkeys.cur=='\0' ) {
         if ( pulsada == false ) {
            keyboard_matrix[ 0x06 >> 4] &= ~bit_values[ 0x06 & 7]; // tecla pulsada -> INTRO
            pulsada = true;
         }else{
            keyboard_matrix[ 0x06 >> 4] |= bit_values[ 0x06 & 7];  // tecla soltada -> INTRO
            pulsada = false;
         }
       }else if( *spoolkeys.cur=='!' ) {
         if ( pulsada == false ) {
           keyboard_matrix[ 0x80 >> 4] &= ~bit_values[ 0x80 & 7]; // tecla pulsada -> "1"
           keyboard_matrix[ 0x25 >> 4] &= ~bit_values[ 0x25 & 7]; // tecla pulsada -> 0x25 (SHIFT)
           pulsada = true;
         }else{
           keyboard_matrix[ 0x80 >> 4] |= bit_values[ 0x80 & 7];  // tecla soltada -> "1"
           keyboard_matrix[ 0x25 >> 4] |= bit_values[ 0x25 & 7];  // tecla soltada -> 0x25 (SHIFT)
           pulsada = false;
         }
       }else if(*spoolkeys.cur=='#'){
         if( pulsada == false ) {
           keyboard_matrix[ 0x71 >> 4] &= ~bit_values[ 0x71 & 7]; // tecla pulsada -> "3"
           keyboard_matrix[ 0x25 >> 4] &= ~bit_values[ 0x25 & 7]; // tecla pulsada -> 0x25 (SHIFT)
           pulsada = true;
         }else{
           keyboard_matrix[ 0x71 >> 4] |= bit_values[ 0x71 & 7];  // tecla soltada -> "3"
           keyboard_matrix[ 0x25 >> 4] |= bit_values[ 0x25 & 7];  // tecla soltada -> 0x25 (SHIFT)
           pulsada=false;
         }
       }else if(*spoolkeys.cur=='+'){
         if(pulsada==false){
           keyboard_matrix[ 0x34 >> 4] &= ~bit_values[ 0x34 & 7]; // tecla pulsada -> "; o ?"
           keyboard_matrix[ 0x25 >> 4] &= ~bit_values[ 0x25 & 7]; // tecla pulsada -> 0x25 (SHIFT)
           pulsada=true;
         }else{
           keyboard_matrix[ 0x34 >> 4] |= bit_values[ 0x34 & 7];  // tecla soltada -> "; o ?"
           keyboard_matrix[ 0x25 >> 4] |= bit_values[ 0x25 & 7];  // tecla soltada -> 0x25 (SHIFT)
           pulsada=false;
         }
       }else if(*spoolkeys.cur=='{'){
         if(pulsada==false){
           keyboard_matrix[ 0x21 >> 4] &= ~bit_values[ 0x21 & 7]; // tecla pulsada -> "{ o [" 
           keyboard_matrix[ 0x25 >> 4] &= ~bit_values[ 0x25 & 7]; // tecla pulsada -> 0x25 (SHIFT)
           pulsada=true;
         }else{
           keyboard_matrix[ 0x21 >> 4] |= bit_values[ 0x21 & 7];  // tecla soltada -> "{ o [" 
           keyboard_matrix[ 0x25 >> 4] |= bit_values[ 0x25 & 7];  // tecla soltada -> 0x25 (SHIFT)
           pulsada=false;
         }
       }else if(*spoolkeys.cur=='}'){
         if(pulsada==false){
           keyboard_matrix[ 0x23 >> 4] &= ~bit_values[ 0x23 & 7]; // tecla pulsada -> "} o ]"
           keyboard_matrix[ 0x25 >> 4] &= ~bit_values[ 0x25 & 7]; // tecla pulsada -> 0x25 (SHIFT)
           pulsada=true;
         }else{
           keyboard_matrix[ 0x23 >> 4] |= bit_values[ 0x23 & 7];  // tecla soltada -> "} o ]"
           keyboard_matrix[ 0x25 >> 4] |= bit_values[ 0x25 & 7];  // tecla soltada -> 0x25 (SHIFT)
           pulsada=false;
         }
       }else if(*spoolkeys.cur=='&'){
         if(pulsada==false){
           keyboard_matrix[ 0x60 >> 4] &= ~bit_values[ 0x60 & 7]; // tecla pulsada -> "6"
           keyboard_matrix[ 0x25 >> 4] &= ~bit_values[ 0x25 & 7]; // tecla pulsada -> 0x25 (SHIFT)
           pulsada=true;
         }else{
           keyboard_matrix[ 0x60 >> 4] |= bit_values[ 0x60 & 7];  // tecla soltada -> "6"
           keyboard_matrix[ 0x25 >> 4] |= bit_values[ 0x25 & 7];  // tecla soltada -> 0x25 (SHIFT)
           pulsada=false;
         }
       }else if(*spoolkeys.cur=='\''){
         if(pulsada==false){
           keyboard_matrix[ 0x51 >> 4] &= ~bit_values[ 0x51 & 7]; // tecla pulsada -> "7"
           keyboard_matrix[ 0x25 >> 4] &= ~bit_values[ 0x25 & 7]; // tecla pulsada -> 0x25 (SHIFT)
           pulsada=true;
         }else{
           keyboard_matrix[ 0x51 >> 4] |= bit_values[ 0x51 & 7];  // tecla soltada -> "7"
           keyboard_matrix[ 0x25 >> 4] |= bit_values[ 0x25 & 7];  // tecla soltada -> 0x25 (SHIFT)
           pulsada=false;
         }
       }else if(*spoolkeys.cur=='^'){ //pone libra
         if(pulsada==false){
           keyboard_matrix[ 0x30 >> 4] &= ~bit_values[ 0x30 & 7]; // tecla pulsada -> "^"
           keyboard_matrix[ 0x25 >> 4] &= ~bit_values[ 0x25 & 7]; // tecla pulsada -> 0x25 (SHIFT)
           pulsada=true;
         }else{
           keyboard_matrix[ 0x30 >> 4] |= bit_values[ 0x30 & 7];  // tecla soltada -> "^"
           keyboard_matrix[ 0x25 >> 4] |= bit_values[ 0x25 & 7];  // tecla soltada -> 0x25 (SHIFT)
           pulsada=false;
         }
       }// falta @
       else if (*spoolkeys.cur=='$'){ //para borrar TEMP!
         if(pulsada==false){
           keyboard_matrix[ 0x97 >> 4] &= ~bit_values[ 0x97 & 7]; // tecla pulsada -> 0x25 (SHIFT)
           pulsada=true;
         }else{
           keyboard_matrix[ 0x97 >> 4] |= bit_values[ 0x97 & 7];  // tecla soltada -> 0x25 (SHIFT)
           pulsada=false;
         }

       }
     }
              
     if(pulsada==false){
       if(spoolkeys.cont>1){
         if(*spoolkeys.cur!=*(spoolkeys.cur+1)){
           spoolkeys.cur++;
           spoolkeys.cont--;
         }else{
           *spoolkeys.cur=0x01;
         }
       }else{
         spoolkeys.cur=&spoolkeys.contenido[0];
         spoolkeys.cont=0;
         spoolkeys.contenido[0]='\0';
         tiempo_spool=0;
       }
     }
  }
}




