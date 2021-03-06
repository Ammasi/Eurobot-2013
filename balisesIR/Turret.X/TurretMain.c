/*
* Turret dsPIC33F
* Compiler : Microchip xC16
* dsPIC33FJ64MC804
* Avril 2013
*    ____________      _           _
*   |___  /| ___ \    | |         | |
*      / / | |_/ /___ | |__   ___ | |_
*     / /  |    // _ \| '_ \ / _ \| __|
*    / /   | |\ \ (_) | |_) | (_) | |_
*   /_/    |_| \_\___/|____/ \___/'\__|
*			      7robot.fr
*/

/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

/* Device header file */
#if defined(__dsPIC33E__)
	#include <p33Exxxx.h>
#elif defined(__dsPIC33F__)
	#include <p33Fxxxx.h>
#endif

#include <stdint.h>        /* Includes uint16_t definition                    */
#include <stdbool.h>       /* Includes true/false definition                  */
#include "TurretUser.h"    /* User funct/params, such as InitApp              */
#include "atp.h"

/******************************************************************************/
/* Global Variable Declaration                                                */
/******************************************************************************/

volatile unsigned char marche;
volatile unsigned int i;
volatile unsigned int j;
//
volatile unsigned char adversaire;
volatile unsigned char distance;
volatile unsigned char direction;
//
volatile unsigned char direction1tab[3];
volatile unsigned char direction2tab[3];
volatile unsigned char distance1tab[3];
volatile unsigned char distance2tab[3];
//
volatile unsigned char direction1;
volatile unsigned char direction2;
volatile unsigned char distance1;
volatile unsigned char distance2;
int adversaire1[taille_uart] = {0,1,0,1,1,1,1,1};
int adversaire2[taille_uart] = {1,0,0,1,1,1,1,1};
int reperage[nombre_recepteurs]; // Contient : 0 non recu, 1 adversaire1, 2 adversaire2 pour chaque TSOP
int donnees[nombre_recepteurs*taille_uart];

volatile unsigned char moy_dist = 0;
volatile unsigned char moy_dir = 0;

/* i.e. uint16_t <variable_name>; */

/******************************************************************************/
/* Main Program                                                               */
/******************************************************************************/
#include <uart.h>
#include <ports.h>
#include <adc.h>
#include <libpic30.h>
#include <pwm12.h>
#include <libpic30.h>
#include <timer.h>
#include "atp-turret.h"

// Prototypes des fonctions
void acquisition(int donnees[]);
int comparer(int donnees[], int recepteur);
void lissage(void);
int who (int reperage[]);
int howFar (int reperage[], int adversaire);
int wichDirection (int reperage[], int adversaire);
void preSendPos(unsigned char adv, unsigned char dist, unsigned char direction);

int16_t main(void)
{
    AD1PCFGL = 0x1FF; // Il est 5h, le robot s eveille
    /* Configure the oscillator for the device */
    ConfigureOscillator();

    /* Initialize IO ports and peripherals */
    InitApp();
    AtpInit();

    marche = 1;
    int recu = 0;

    int y;
    for (y=0;y<10;y++)
    {
        led1 = !led1;
        led2 = !led1;
        __delay_ms(200);
    }
    __delay_ms(1000);
	led1 = 0;
	led2 = 0;
    SendId(BOARD_ID);

    while(1)
    {
		// Detection du bit de Start
        if (!TSOP1 || !TSOP2 || !TSOP3 || !TSOP4 || !TSOP5 || !TSOP6 || !TSOP7 || !TSOP8 || 
            !TSOP9 || !TSOP10 || !TSOP11 || !TSOP12 || !TSOP13 || !TSOP14 || !TSOP15 || !TSOP16)
        {
            __delay_us(bit_periode_us/2); // Se place au millieu du bit de Start
            acquisition(donnees);
            __delay_us(bit_periode_us); // Sort de la trame envoyee
            for(j = 0 ; j < nombre_recepteurs ; j++)
            {
        	reperage[j] = comparer(donnees, j);
            }
            lissage(); // Corrige les recepteurs defaillants
            adversaire = who(reperage); // Indique si on vient de recevoir adversaire 1 ou 2
            if (!adversaire) continue; // Cas de fausse detection

       	// DISTANCE ET DIRECTION /////////////////////////////////////////////////////
            int k = 0, first = 0, last = 0, distance = 0, direction = 0;
            if (!reperage[0]) // TSOP0 ne recoit rien
            {
            	while(reperage[k] != adversaire && k < nombre_recepteurs) k++;
            	first = k;
            	while(reperage[k] == adversaire && k < nombre_recepteurs) k++;
            	last = k;
            	distance = last - first;
            	direction = (last + first) /2;
            }
            else if (!reperage[nombre_recepteurs-1]  && reperage[0] == adversaire) // TSOP0 est first
            {
            	k++;
            	while(reperage[k] == adversaire && k < nombre_recepteurs) k++;
            	// last = k;
            	distance = k;
                direction = k /2;
            }
            else	// TSOP0 est entre first et last
            {
    		while(reperage[k] == adversaire && k < nombre_recepteurs) k++;
		last = k;
		while(reperage[k] != adversaire && k < nombre_recepteurs) k++;
		first = k;
		distance = (last + nombre_recepteurs - first) % nombre_recepteurs;
		direction = ((last + nombre_recepteurs + first) % (2*nombre_recepteurs)) /2;
            }
	// FIN DISTANCE ET ANGLE //////////////////////////////////////////////
			
            switch(adversaire)
            {
            case 0 :
            break;

            case 1 : // On vient de detecter adversaire 1
		distance1tab[2] = distance1tab[1];
		direction1tab[2] = direction1tab[1];
		distance1tab[1] = distance1tab[0];
		direction1tab[1] = direction1tab[0];
		distance1tab[0] = distance;
		direction1tab[0] = direction;
		if(((distance1tab[2] != distance1tab[1]) || (direction1tab[2] != direction1tab[1])) && distance1tab[0] == distance1tab[1] && direction1tab[0] == direction1tab[1])
		{
                    if (marche)	preSendPos1(1, distance, direction);
                    distance1 = distance;
                    direction1 = direction;
		}
		else
		{
                    break;
		}
            break;

            case 2 : // On vient de detecter adversaire 2
                if (distance == 0) break;
                distance2tab[2] = distance2tab[1];
                direction2tab[2] = direction2tab[1];
                distance2tab[1] = distance2tab[0];
                direction2tab[1] = direction2tab[0];
                distance2tab[0] = distance;
                direction2tab[0] = direction;
                if(((distance2tab[2] != distance2tab[1]) || (direction2tab[2] != direction2tab[1])) && distance2tab[0] == distance2tab[1] && direction2tab[0] == direction2tab[1])
                {
                    if (marche)	preSendPos2(2, direction, distance);
                    distance2 = distance;
                    direction2 = direction;
                }
                else
                {
                    break;
                }
		break;

            default:
                break;
            }
            recu = 0;
        }
    }
}

// FONCTIONS DE TRAITEMENT ///////////////////////////////////////////////

void acquisition(int donnees[])
{
    for(i = 0 ; i<taille_uart ; i++)
    {
	__delay_us(bit_periode_us); // Passe au bit suivant
    donnees[i] = TSOP1;
    donnees[taille_uart+i] = TSOP2;
	donnees[2*taille_uart+i] = TSOP3;
	donnees[3*taille_uart+i] = TSOP4;
	donnees[4*taille_uart+i] = TSOP5;
	donnees[5*taille_uart+i] = TSOP6;
	donnees[6*taille_uart+i] = TSOP7;
	donnees[7*taille_uart+i] = TSOP8;
	donnees[8*taille_uart+i] = TSOP9;
	donnees[9*taille_uart+i] = TSOP10;
	donnees[10*taille_uart+i] = TSOP11;
	donnees[11*taille_uart+i] = TSOP12;
	donnees[12*taille_uart+i] = TSOP13;
	donnees[13*taille_uart+i] = TSOP14;
	donnees[14*taille_uart+i] = TSOP15;
	donnees[15*taille_uart+i] = TSOP16;
    }
}

int comparer(int donnees[], int recepteur)
{
    int adversaire = 0;
    // Detection Adversaire 1
    for(i = 0 ; i < taille_uart-1 ; i++)
    {
        if(donnees[recepteur*taille_uart+i] != adversaire1[i]) break;
    }
	if(i == taille_uart-1 && donnees[recepteur*taille_uart+i] == adversaire1[i])
	{
		adversaire = 1;
        led1 = 1;
        led2 = 0;
	}
	// Detection Adversaire 2
    for(i = 0 ; i<taille_uart-1; i++)
    {
        if(donnees[recepteur*taille_uart+i] != adversaire2[i]) break;
    }
	if(i == taille_uart-1 && donnees[recepteur*taille_uart+i] == adversaire2[i])
	{
		adversaire = 2;
        led1 = 0;
        led2 = 1;
	}
    return adversaire;
}

void lissage(void)
{
	if((reperage[nombre_recepteurs-1] != 0) && (reperage[nombre_recepteurs-1] == reperage[1]) && (reperage[0] != reperage[nombre_recepteurs-1]))
	{
		reperage[0] = reperage[nombre_recepteurs-1];
	}
	for(i= 0 ; i < nombre_recepteurs-2 ; i++)
	{
		if((reperage[i] != 0) && (reperage[i] == reperage[i+2]) && (reperage[i+1] != reperage[i])) 
		{
			reperage[i+1] = reperage[i];
		}
	}
	if((reperage[nombre_recepteurs-2] != 0) && (reperage[nombre_recepteurs-2] == reperage[0]) && (reperage[nombre_recepteurs-1] != reperage[nombre_recepteurs-2])) 
	{
		reperage[nombre_recepteurs-1] = reperage[nombre_recepteurs-2];
	}
}

int who (int reperage[])
{
	int k = 0, sortie = 0;
	while(!reperage[k] && k < nombre_recepteurs) k++;
	sortie = reperage[k];
	return sortie;
}

unsigned char pre_dir1;
unsigned char pre_dist1;

void preSendPos1(unsigned char adv, unsigned char dist, unsigned char dir)
{
    if ((pre_dir1 == dir || (pre_dir1-1)%16 == dir || (pre_dir1+1)%16 == dir || abs(pre_dist1 - dist) > 5)
        && (pre_dist1 == dist || pre_dist1-1 == dist || pre_dist1+1 == dist || abs((pre_dir1 - dir)%16) > 5)) return;
    
    SendPos(adv, dist, dir);
    pre_dir1 = dir;
    pre_dist1 = dist;
}

unsigned char pre_dir2;
unsigned char pre_dist2;

void preSendPos2(unsigned char adv, unsigned char dist, unsigned char dir)
{
    if ((pre_dir2 == dir || (pre_dir2-1)%16 == dir || (pre_dir2+1)%16 == dir || abs(pre_dist2 - dist) > 5)
        && (pre_dist2 == dist || pre_dist2-1 == dist || pre_dist2+1 == dist || abs((pre_dir2 - dir)%16) > 5)) return;

    SendPos(adv, dist, dir);
    pre_dir2 = dir;
    pre_dist2 = dist;
}

// FONCTIONS ATP ///////////////////////////////////////////////

void OnOn ()
{
	marche = 1;
}

void OnOff ()
{
	marche = 0;
}

void OnGetPos(unsigned char id)
{
    if (id == 1)		SendPos(1, distance1, direction1);
	else			SendPos(2, distance2, direction2);
}
