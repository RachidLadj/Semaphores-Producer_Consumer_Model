#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define Color(param) printf("\033[%sm",param)

#define nbsem 7
#define tailletompon 2
#define mutex1 0
#define buffer 1
#define vide 2
#define plein1 3
#define mutex2 4
#define plein2 5
#define ecran 6


int id; char p;
union semun{
	int semval;		//pour OPTION = SETVALL
	struct semid_ds *buf;	//pour OPTION = IPC_SET & IPC_STAT
	ushort *array;		//pour OPTION = SETALL & GETALL
}arg_ctl;

union semun semval;	 // pour init valdun sem ou d'un enemble de sem
struct sembuf operation; // pour le P(sem) et V(sem) 

typedef struct code{
	int codemodule;
	float moyenne;
}code;


typedef struct sdata{
	int nb1,nb2;
	int q,t;
	code arraycode[tailletompon] ;
}sdata;


//void Color(int couleurDuTexte,int couleurDeFond);
int create_segment (key_t cle,size_t taille);			//Création d'un segment ou  récupération de l'Id du segment deja créé
int create_sem (key_t cle,int nbsems);				//Création d'un ensemble de sémaphore ou récupération de l'Id de l'ensemle déja créé
int init_sem(int semid,unsigned short int  semvals[]);		//initialisation  d'un ensemble de sémaphores avec des valeurs introduit dans le tableau semvals
void Producteur1(int semid,int sgmid,sdata* adr);		//la methode que le producteur1 appelle 
void Producteur2(int semid,int sgmid,sdata* adr);		//la methode que le producteur2 appelle 
code produit(FILE* F);						//Le producteur Produit (Lis du fichier)
void deposer(code obj,sdata* adr);				//Le producteur Dépose dans le tompon
void P(int semid,int numsem);					//Primitive P des sémaphores
void V(int semid,int numsem);					//Primitive V des sémaphores
int consomateur1(int semid,int sgmid,sdata* adr,int nb);	//la methode que le consomateur1 appelle 
int consomateur2(int semid,int sgmid,sdata* adr,int nb);	//la methode que le consomateur2 appelle 
code prelever(sdata* adr);					//Le consameteur préleve du tompon
void consomer (FILE* F,code obj);				//le consomateur consome (ecrit dans le fichier)
int supp_sem(int semid);					//Destruction de l'ensemble des sémaphore
void AfficherSem(int semid);					//Affiche l'etat de chaque semaphore


int main(){
	size_t taille = sizeof(code);
	char* pathname="/home/rashid/Bureau/TP SYSTEM/TPIPC1"; 
/* recup la cle */  
	key_t cle = ftok(pathname,100);
	printf("key_t cle = %d \n",(int)cle);
	
/* creer l'ensemble des semaphore */
	int semid = create_sem(cle,nbsem);
	printf("semget --> semid = %d \n",semid);

/* créer le segment */
	int sgmid= create_segment(cle,sizeof(sdata));
	printf("shmget --> sgmid = %d \n",sgmid);
/*attacher le segment */
	sdata* adr = NULL;
	adr = shmat(sgmid,adr,0);   /*renvoiel'adresse oul'attahcment à été effectivement réalisé.*/
	if(adr == (sdata*)-1){
		perror("Erreur ...");
		exit(-1);	
	}
	adr->q=0;
	adr->t=0;
	adr->nb1=0;
	adr->nb2=0;
	
	unsigned short int val[nbsem];
	val[mutex1] = 1;		//Semaphore Binaire
	val[buffer] = 1;		//Semaphore Binaire
	val[vide] = tailletompon; 	//Semaphore Binaire initialiséà 20
	val[plein1] = 0;		//Semaphore Privée
	val[mutex2] = 1;		//Semaphore Binaire
	val[plein2] = 0;		//Semaphore Privée
	val[ecran] = 1;			//Semaphore Binaire
	init_sem(semid,val);
	AfficherSem(semid);


/******************************************************************************************/


id = fork();
if(id == 0){ 
	Producteur1(semid,sgmid,adr);
printf("tetpp");
}
id = fork();
if(id==0) { 
	Producteur2(semid,sgmid,adr);
}
id = fork();	
if(id==0) { 
	FILE* F = fopen("F3.txt","w+");
	fclose(F);
	int nb=0,x;	
	do{
		nb++;
		x = consomateur1(semid,sgmid,adr,nb);
	}while (x==0);
	Color("5;31");
	printf("\n\n		/***************** exit Consomateur 1 *****************/\n\n");
	Color("40;");
	exit(0);
}
id = fork();
if(id==0) {
	FILE* F = fopen("F4.txt","w+");
	fclose(F);
	int nb=0;
	int x; 
	do{
		nb++;
		x = consomateur2(semid,sgmid,adr,nb);
	}while(x==0);
	Color("5;31");
	printf("\n\n		/***************** exit Consomateur 2 *****************\n\n");
	Color("40;");
	exit(0);
}
wait(NULL);
wait(NULL);
wait(NULL);
wait(NULL);
printf("\nL'état des sémaphores a la fin du programme\n");
AfficherSem(semid);
/* supprimer le nsemble des sémaphore */
	supp_sem (semid);	
/* déttacher le segment */
	shmdt(adr);
/* supprimer le segment */
	shmctl(sgmid,IPC_RMID,0);
		
printf("\n\n\nFin de programme\n");
}


void Producteur1(int semid,int sgmid,sdata* adr){
	//char p;
	FILE* F= NULL;
	F = fopen("F1.txt","r");
	int t,j,nb=0;
	do{
		sleep(2);
		code c = produit(F);
		Color("0;34");
		printf("\nProducteur 1   produit l'objet (%d,%f)	identifié par Message %d",c.codemodule,c.moyenne,nb+1);
		Color("40;");
		P(semid,mutex1);
		//AfficherSem(semid);
		if(adr->nb1==0)
			P(semid,buffer);
		adr->nb1++;
		V(semid,mutex1);
		P(semid,vide);
		P(semid,ecran);
	semctl(semid,nbsem/*0*/,GETALL,semval);
	printf("\n	Producteur 1:juste avant \"déposer\" la valeur du sémaphore ecran est  %hu",semval.array[ecran]);
		deposer(c,adr);
		Color("0;34");	
		printf("\nProducteur 1   dépose l'objet (%d,%f)	identifié par Message %d",c.codemodule,c.moyenne,nb+1);
		Color("40;");
	
		V(semid,ecran);
	semctl(semid,nbsem/*0*/,GETALL,semval);
	printf("\n	Producteur 1:juste apres \"déposer\" la valeur du sémaphore ecran est  %hu\n",semval.array[ecran]);
		V(semid,plein1);
		//AfficherSem(semid);
		nb++;
	}while (p!=EOF);
	fclose(F);
	Color("5;31");
	printf("\n\n		/***************** exit producteur 1 *****************/\n\n");
	Color("40;");
	exit(0);
}

void Producteur2(int semid,int sgmid,sdata* adr){
	FILE* F= NULL;
	F = fopen("F2.txt","r");
	int t,j,nb=0;
	do{
		sleep(4);
		code c = produit(F);
		Color("0;32");
		printf("\nProducteur 2   produit l'objet (%d,%f)	identifié par Message %d",c.codemodule,c.moyenne,nb+1);
		Color("40;");
		P(semid,mutex2);
		//AfficherSem(semid);
		if(adr->nb2==0)
			P(semid,buffer);
		adr->nb2++;
		V(semid,mutex2);
		P(semid,vide);
		P(semid,ecran);
	semctl(semid,nbsem/*0*/,GETALL,semval);
	printf("\n	Producteur 2:juste avant \"déposer\" la valeur du sémaphore ecran est  %hu",semval.array[ecran]);
		deposer(c,adr);
		Color("0;32");	
		printf("\nProducteur 2   dépose l'objet (%d,%f)	identifié par Message %d",c.codemodule,c.moyenne,nb+1);
		Color("40;");
	
		V(semid,ecran);
	semctl(semid,nbsem/*0*/,GETALL,semval);
	printf("\n	Producteur 2:juste apres \"déposer\" la valeur du sémaphore ecran est  %hu\n",semval.array[ecran]);
		V(semid,plein2);
		//AfficherSem(semid);
		nb++;
	}while (p!=EOF);	
	fclose(F);
	Color("5;31");
	printf("\n\n		/***************** exit producteur 2 *****************/\n\n");
	Color("40;");
	exit(0);
}


code produit(FILE* F){
	int t; float t1;
	code info;
	fscanf(F,"%d",&t);
 	p = fgetc(F);
	fscanf(F,"%3f",&t1);
	if (t == 0) {t=-1; t1 = -1;}
	info.codemodule = t;
	info.moyenne = t1;
	p = fgetc(F);
	return info;
}

void deposer(code obj,sdata* adr){
	adr->arraycode[adr->t]= obj;
	adr->t= (adr->t+1)%tailletompon;
}

int consomateur1(int semid,int sgmid,sdata* adr,int nb){
	FILE* F = fopen("F3.txt","a");
	P(semid,plein1);
	P(semid,ecran);
		semctl(semid,nbsem/*0*/,GETALL,semval);
		printf("\n	Consomateur 1:juste avant \"prélver\" la valeur du sémaphore ecran est  %hu",semval.array[ecran]);
	code obj = prelever(adr);
	Color("1;34");
	printf("\nConsomateur 1   préleve l'objet (%d,%f) 	identifié par Message %d",obj.codemodule,obj.moyenne,nb);
	Color("40;");
	V(semid,ecran);	
		semctl(semid,nbsem/*0*/,GETALL,semval);
		printf("\n	Consomateur 1:juste apres \"prélver\" la valeur du sémaphore ecran est  %hu",semval.array[ecran]);
	V(semid,vide);
	P(semid,mutex1);
	adr->nb1--;
	if (adr->nb1 == 0) 
		V(semid,buffer);
	V(semid,mutex1);
	if(obj.codemodule!=-1){
	P(semid,ecran);
		semctl(semid,nbsem/*0*/,GETALL,semval);
		printf("\n	Consomateur 1:juste avant \"Consommer\" la valeur du sémaphore ecran est  %hu",semval.array[ecran]);
		consomer(F,obj);
		Color("1;34");
		printf("\nConsomateur 1 consomme l'objet (%d,%f) 	identifié par Message %d",obj.codemodule,obj.moyenne,nb);
		Color("40;");
	V(semid,ecran);
		semctl(semid,nbsem/*0*/,GETALL,semval);
		printf("\n	Consomateur 1:juste apres \"Consommer\" la valeur du sémaphore ecran est  %hu",semval.array[ecran]);
	}
	fclose(F);
	if(obj.codemodule!=-1)
		return 0;
	else
		return -1;
}

int consomateur2(int semid,int sgmid,sdata* adr,int nb){
	FILE* F = fopen("F4.txt","a");
	P(semid,plein2);
	P(semid,ecran);
		semctl(semid,nbsem/*0*/,GETALL,semval);
		printf("\n	Consomateur 2:juste avant \"prélver\" la valeur du sémaphore ecran est  %hu",semval.array[ecran]);
	code obj = prelever(adr);
	Color("1;32");
	printf("\nConsomateur 2   préleve l'objet (%d,%f) 	identifié par Message %d",obj.codemodule,obj.moyenne,nb);
	Color("40;");
	V(semid,ecran);	
		semctl(semid,nbsem/*0*/,GETALL,semval);
		printf("\n	Consomateur 2:juste apres \"prélver\" la valeur du sémaphore ecran est  %hu",semval.array[ecran]);
	V(semid,vide);
	P(semid,mutex2);
	adr->nb2--;
	if (adr->nb2 == 0) 
		V(semid,buffer);
	V(semid,mutex2);
	if(obj.codemodule!=-1){
	P(semid,ecran);
		semctl(semid,nbsem/*0*/,GETALL,semval);
		printf("\n	Consomateur 2:juste avant \"Consommer\" la valeur du sémaphore ecran est  %hu",semval.array[ecran]);
		consomer(F,obj);
		Color("1;32");
		printf("\nConsomateur 2 consomme l'objet (%d,%f) 	identifié par Message %d",obj.codemodule,obj.moyenne,nb);
		Color("40;");
	V(semid,ecran);
		semctl(semid,nbsem/*0*/,GETALL,semval);
		printf("\n	Consomateur 2:juste apres \"Consommer\" la valeur du sémaphore ecran est  %hu",semval.array[ecran]);
	}
	fclose(F);
	if(obj.codemodule!=-1)	
		return 0;
	else 
		return -1;
}

code prelever(sdata* adr){
	code obj = adr->arraycode[adr->q];
	adr->q= (adr->q+1)%tailletompon;
	return obj;
}

void consomer (FILE* F,code obj){
	fprintf(F,"%d %f\n",obj.codemodule,obj.moyenne);

}




/*	creation d'un segment mémoire partagé	*/
int create_segment(  key_t cle,		//clé de segment
		     size_t taille	//taille du segment (dans notre cas 2*taille d'un int
		     /*int option*/	//Option de création
	          ){
	int sgmid = shmget(cle,taille,IPC_CREAT|IPC_EXCL|0666);
	if(sgmid == -1){ 	//existe déja on retourne le sgmid pour qu'il puisse etre utilisé
		sgmid = shmget(cle,taille,0);	
	}
	return sgmid;
}

/*	Création d'un ensemble de sémaphores		*/
int create_sem(	key_t cle,		// clé utilisateur
			int nbsems		// nombre de sémaphore
			/*int option,*/		//option de création
		    ){
	int semid = semget(cle,nbsems,IPC_CREAT|IPC_EXCL|0666);
	printf("test 1 : semid = %d\n",semid);	
	if(semid == -1){ //existe déja on retourne le sgmid pour qu'il puisse etre utilisé
		semid = semget(cle,nbsems,0);
		printf("test 2 : semid = %d\n",semid);	
	}
	return semid;
}

/*	initialisation des sémaphores appartenant à un ensemble		*/
int init_sem(int semid,unsigned short int  semvals[]){
	semval.array = semvals;
	return semctl(semid,nbsem/*0*/,SETALL,semval);	//Affecté le tableau contenant les val initials des semaphore à lunion
}


void P(int semid,int numsem){
	operation.sem_num = numsem;
	operation.sem_op = -1;
	operation.sem_flg = 0;
	semop (semid, &operation,1);

}

void V(int semid,int numsem){
	operation.sem_num = numsem;
	operation.sem_op = 1;
	operation.sem_flg = 0;
	semop (semid, &operation,1);
}
 
int supp_sem(int semid){
	return semctl(semid,0,IPC_RMID,0);
}

void AfficherSem(int semid){
	P(semid,ecran);
	Color("0;36");		//Trés beau Bleu
	semctl(semid,nbsem/*0*/,GETALL,semval);
	printf("	╔═══════════════════════════════════════════════════════╗\n");
	printf("	║	la valeur du sémaphore mutex1 est  %hu		║\n",semval.array[mutex1]);
	printf("	║	la valeur du sémaphore buffer est  %hu		║\n",semval.array[buffer]);
	printf("	║	la valeur du sémaphore vide est  %hu		║\n",semval.array[vide]);
	printf("	║	la valeur du sémaphore plein1 est  %hu		║\n",semval.array[plein1]);
	printf("	║	la valeur du sémaphore mutex2 est  %hu		║\n",semval.array[mutex2]);
	printf("	║	la valeur du sémaphore plein2 est  %hu		║\n",semval.array[plein2]);
	printf("	║	la valeur du sémaphore ecran est  %hu 		║\n",semval.array[ecran]);
	printf("	║		(initialisé à 1 comme on est entrain    ║\n");
	printf("	║		d'afficher un message on a appliqué un  ║\n");
	printf("	║		P(ecran) se qui a modifié sa valeur de 	║\n");
	printf("	║		1 à 0.)					║\n");
	printf("	╚═══════════════════════════════════════════════════════╝");
	Color("40;");
	V(semid,ecran);
}




