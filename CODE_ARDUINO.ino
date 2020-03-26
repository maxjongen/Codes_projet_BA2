#include <Servo.h>
#include <SoftwareSerial.h>

Servo moteur_principal;    //moteur à rotation continue
Servo pince_bas;
Servo pince_haut;

SoftwareSerial bluetooth(10, 11); // le pin 10 de l'Arduino se transforme en pin RX pour l'Arduino, le pin 11 se transforme en pin TX pour l'Arduino
//                                !!!!!! BRANCHER LE PIN RX DU MODULE BLUETOOTH SUR LE PIN TX DE L'ARDUINO ET LE PIN TX DU MODULE BT SUR LE PIN RX DE L'ARDUINO !!!!!!

//Constantes définies ici (--> plus simple de modifier ici plutôt que partout dans le code)

// < CONSTANTES POUR LES PINS >
#define MOTEUR_PRINCIPAL 2
#define PINCE_HAUT 3
#define PINCE_BAS 4

#define CONTACT_HAUT 5
#define CONTACT_BAS 6
#define CONTACT_CIEL 7
#define CONTACT_TERRE 8

#define INFRA_HAUT A0
#define INFRA_BAS A1
#define INFRA_CIEL A2
//                                    Haut et bas --> concerne ce qui se passe au niveau des pinces
//                                    Ciel et terre --> concerne ce qui se passe au niveau des extrémités du robot (tout en haut / tout en bas)

// <AUTRES CONSTANTES>
const int LIMITE_INFRA = 100;  //un capteur infra detectera si sa mesure est inférieure à cette valeur
const int LIMITE_ROTATION_PINCE = 100;   // permettra d'éviter des dégradations à cause d'une rotation trop grande de la pince
const int t = 60;  //permettra de contrôler la vitesse de rotation des petits servos
const int attente = 250; //pour éviter que l'infra se remette directement à détecter quand le robot recommence à monter/descendre juste après la rotation de la pince
const int temps_limite_montee = 2000; //pour qu'il comprenne qu'il a dépassé le dernier échelon  !!! <<< CF TESTS >>> !!!                                                                                   !!!!!!!!!!!!!!!!!!

const String STOP = "0\r\n";
const String MONTEE = "1\r\n";
const String DESCENTE = "2\r\n";



String instruction;



void setup() {  // exécuté une seule fois au tout début 

  //Moteurs
  moteur_principal.attach(0);
  pince_bas.attach(1);
  pince_haut.attach(2);

  //Capteurs infrarouges
  pinMode(INFRA_HAUT, INPUT);
  pinMode(INFRA_BAS, INPUT);
  pinMode(INFRA_CIEL, INPUT);

  //Capteurs de contact
  pinMode(CONTACT_HAUT,INPUT);
  pinMode(CONTACT_BAS,INPUT);
  pinMode(CONTACT_CIEL,INPUT); 
  pinMode(CONTACT_TERRE,INPUT); 

  //Voie série avec le module Bluetooth
  bluetooth.begin(38400);

  instruction = ""; //pas d'instruction à la base
  //                  1 --> montée  ;  2 --> descente  ;  0(=STOP) --> arrêt
   
}



void loop() { //répété en boucle
  if (bluetooth.available()>0){
    instruction = bluetooth.readString();
  }

  // <<< CODE POUR LA MONTEE >>> --> c'est que le robot commence vers le bas
  
  while(instruction == MONTEE){   
    
    //Booléens liés aux capteurs pour voir s'ils détectent ou pas :                  si c'est true, le capteur est en train de détecter
    boolean detecte_infra_haut;
    boolean detecte_infra_bas;
    boolean detecte_infra_ciel = 28500/(analogRead(INFRA_CIEL)) < LIMITE_INFRA;
    boolean detecte_contact_haut = (digitalRead(CONTACT_HAUT) == 1);
    boolean detecte_contact_bas = (digitalRead(CONTACT_BAS) == 1);
    boolean detecte_contact_ciel = (digitalRead(CONTACT_CIEL) == 1);
    boolean detecte_contact_terre = (digitalRead(CONTACT_TERRE) == 1);
    
    if(detecte_contact_bas){      // MOUVEMENT DE LA PARTIE "HAUT"
      int rotation_ph = pince_haut.read();
      //ce for permet de décrocher la pince du haut
      for(rotation_ph ; rotation_ph > 0; rotation_ph--)  // !!!!!!!!!!!!!!!!!!!!!! valeurs à changer, cf tests !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      {
        pince_haut.write(rotation_ph);          //ici ".write(angle)" permet que le servo aille à un angle précis (ce ne sera pas pareil pour le moteur principal, qui est à rotation continue)
        delay(t);
      }
      moteur_principal.write(0);  //la fonction .write(...) permet que ça tourne en continu dans un sens, l'argument permet de contrôler la vitesse (0 pour un sens, 180 pour l'autre, 94 pour l'arrêt) (pas pareil que pour les petits servos!)
      int temps_initial = millis();
      delay(attente); //pour que ça ne détecte pas direct
      detecte_infra_haut = false;     
      while(!(detecte_infra_haut or detecte_infra_ciel or detecte_contact_ciel or (millis()-temps_initial) > temps_limite_montee)){     //le moteur tourne tant que l'échelon n'est pas détecté et tant qu'il n'y a pas de problème
        detecte_infra_haut = 28500/(analogRead(INFRA_HAUT)) < LIMITE_INFRA; 
        detecte_contact_ciel = (digitalRead(CONTACT_CIEL) == 1);
        detecte_infra_ciel = 28500/(analogRead(INFRA_CIEL)) < LIMITE_INFRA;
      }
      moteur_principal.writeMicroseconds(1510);  // on stoppe d'office le moteur
      
      if(!(detecte_contact_ciel or detecte_infra_ciel) and detecte_infra_haut){  //ne sera pas lu si un problème est survenu (--> stop) ou si on est arrivé tout en haut (--> début de l'algorithme pour descendre)
        detecte_contact_haut = (digitalRead(CONTACT_HAUT) == 1);
        rotation_ph = pince_haut.read();
        while( ! detecte_contact_haut and rotation_ph <= LIMITE_ROTATION_PINCE ){  //ce while permet que la pince tourne tant que le contact avec l'échelon n'est pas détecté
          pince_haut.write(rotation_ph);
          rotation_ph+=1;
          detecte_contact_haut = (digitalRead(CONTACT_HAUT) == 1);
          delay(t);                                //petit t --> grande vitesse de rotation
        }
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! if à ajouter ici --> instruction = STOP si limite dépassée !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        if(detecte_contact_haut){       //MOUVEMENT DE LA PARTIE "BAS", la logique est exactement la même que pour la partie du haut
          int rotation_pb = pince_bas.read();
          for(rotation_pb ; rotation_pb > 0; rotation_pb--)  //valeurs peut-être fausses, à déterminer quand on fera des tests
          {
            pince_bas.write(rotation_pb);
            delay(t);
          }
          moteur_principal.write(180);           // Le moteur principal tourne dans l'autre sens
          delay(attente);
          detecte_infra_bas = false;
          while(!detecte_infra_bas){             
            detecte_infra_bas = 28500/(analogRead(INFRA_BAS)) < LIMITE_INFRA;
          }
          moteur_principal.writeMicroseconds(1510);
          detecte_contact_bas = (digitalRead(CONTACT_BAS) == 1);
          rotation_pb = pince_bas.read();
          while( ! detecte_contact_bas and rotation_pb <= LIMITE_ROTATION_PINCE ){  
            pince_bas.write(rotation_pb);
            rotation_pb+=1;
            detecte_contact_bas = (digitalRead(CONTACT_BAS) == 1);
            delay(t);
          }
        }        
      }
      else if (detecte_contact_ciel or detecte_infra_ciel){
        instruction = STOP;
      }
      else{
        instruction = DESCENTE;
      }
    }
    
    else if (!detecte_contact_bas and detecte_contact_haut){    //tentative de correction du problème : même code que précédemment, pour remettre la pince correctement.
      int rotation_pb = pince_bas.read();
      for (rotation_pb ; rotation_pb > 0 ; rotation_pb--){
        pince_bas.write(rotation_pb);
        delay(t);
      }
      moteur_principal.write(180);           
      delay(attente);
      detecte_infra_bas = false;
      while(!detecte_infra_bas){             
        detecte_infra_bas = 28500/(analogRead(INFRA_BAS)) < LIMITE_INFRA;
      }
      moteur_principal.writeMicroseconds(1510);
      detecte_contact_bas = (digitalRead(CONTACT_BAS) == 1);
      rotation_pb = pince_bas.read();
      while( ! detecte_contact_bas and rotation_pb <= LIMITE_ROTATION_PINCE ){  
        pince_bas.write(rotation_pb);
        rotation_pb+=1;
        detecte_contact_bas = (digitalRead(CONTACT_BAS) == 1);
        delay(t);
      }      
    }
    
    else{
      instruction = STOP;
    }
  }


  // <<< CODE POUR LA DESCENTE >>> --> c'est que le robot commence vers le haut
  while(instruction == DESCENTE){
    
    boolean detecte_infra_haut;
    boolean detecte_infra_bas;
    boolean detecte_infra_ciel = analogRead(INFRA_CIEL) < LIMITE_INFRA;
    boolean detecte_contact_haut = (digitalRead(CONTACT_HAUT) == 1);
    boolean detecte_contact_bas = (digitalRead(CONTACT_BAS) == 1);
    boolean detecte_contact_ciel = (digitalRead(CONTACT_CIEL) == 1);
    boolean detecte_contact_terre = (digitalRead(CONTACT_TERRE) == 1);
    
    if(detecte_contact_haut){      // MOUVEMENT DE LA PARTIE "BAS"
      int rotation_pb = pince_bas.read();
      for(rotation_pb ; rotation_pb > 0; rotation_pb--)
      {
        pince_bas.write(rotation_pb);
        delay(t);
      }
      moteur_principal.write(0);  //montée pince du haut BLOQUEE  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! PROBLEME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!! PROBLEME !!!!!!!!!!!!!!!!!!!!!!!!!!! PROBLEME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      delay(attente);
      detecte_infra_bas = false;
      while(!(detecte_infra_bas or detecte_contact_terre)){
        detecte_infra_bas = 28500/(analogRead(INFRA_BAS)) < LIMITE_INFRA; 
        detecte_contact_terre = (digitalRead(CONTACT_TERRE) == 1);
      }
      moteur_principal.writeMicroseconds(1510);
      if(!(detecte_contact_terre) and detecte_infra_bas){  //ne sera pas lu si un problème est survenu (--> stop) ou si on est arrivé tout en bas (--> largage de charge --> début de la montée)
        detecte_contact_bas = (digitalRead(CONTACT_BAS) == 1);
        rotation_pb = pince_bas.read();
        while( ! detecte_contact_bas and rotation_pb <= LIMITE_ROTATION_PINCE ){
          pince_bas.write(rotation_pb);
          rotation_pb+=1;
          detecte_contact_bas = (digitalRead(CONTACT_BAS) == 1);
          delay(t);                                
        }
        if(detecte_contact_bas){       // <<< MOUVEMENT DE LA PARTIE "HAUT" >>>
          int rotation_ph = pince_haut.read();
          for(rotation_ph ; rotation_ph > 0; rotation_ph--){
            pince_haut.write(rotation_ph);
            delay(t);
          }
          moteur_principal.write(180);           // descente de la pince du haut
          delay(attente);
          detecte_infra_haut = false;
          while(!detecte_infra_haut){             
            detecte_infra_haut = 28500/(analogRead(INFRA_HAUT)) < LIMITE_INFRA;
          }
          moteur_principal.writeMicroseconds(1510);
          detecte_contact_haut = (digitalRead(CONTACT_HAUT) == 1);
          rotation_ph = pince_haut.read();
          while( ! detecte_contact_haut and rotation_ph <= LIMITE_ROTATION_PINCE ){  
            pince_haut.write(rotation_ph);
            rotation_ph+=1;
            detecte_contact_haut = (digitalRead(CONTACT_HAUT) == 1);
            delay(t);
          }
        }
      }
      else if (detecte_contact_terre){
        instruction = MONTEE;                                 //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! SANS DOUTE CHANGEMENTS A FAIRE ICI, CF TESTS !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      }
    }
    else if (!detecte_contact_haut and detecte_contact_bas){    //tentative de correction du problème : même code que précédemment, pour remettre la pince correctement.
      int rotation_ph = pince_haut.read();
      for (rotation_ph ; rotation_ph > 0 ; rotation_ph--){
        pince_haut.write(rotation_ph);
        delay(t);
      }
      moteur_principal.write(180);           
      delay(attente);
      detecte_infra_haut = false;
      while(!detecte_infra_haut){             
        detecte_infra_haut = 28500/(analogRead(INFRA_HAUT)) < LIMITE_INFRA;
      }
      moteur_principal.writeMicroseconds(1510);
      detecte_contact_haut = (digitalRead(CONTACT_HAUT) == 1);
      rotation_ph = pince_haut.read();
      while( ! detecte_contact_haut and rotation_ph <= LIMITE_ROTATION_PINCE ){  
        pince_haut.write(rotation_ph);
        rotation_ph+=1;
        detecte_contact_haut = (digitalRead(CONTACT_HAUT) == 1);
        delay(t);
      }      
    }
    
    else{
      instruction = STOP;
    }      
  }

     
}     //                           A AJOUTER : faire en sorte que tout s'arrête si rotation_ph dépasse la limite   
