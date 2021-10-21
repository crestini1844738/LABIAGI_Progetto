#include "ros/ros.h"
#include "pick_and_delivery/UserLogin.h"
#include "pick_and_delivery/ControlSendOrRec.h"
#include "pick_and_delivery/ControlRobotReady.h"
#include "pick_and_delivery/Spedizione.h"
#include "pick_and_delivery/SpedizioneRobot.h"

#include "pick_and_delivery/InfoComunication.h" //due campi: status e info
												//status 0: ancora in esecuzione
												//status 1: terminato con successo; info con messaggio di successo
												//status -1: terminato con errore; info errore
//#include "set_goal/NewGoal.h"
#include "geometry_msgs/PoseStamped.h"
#include "tf/tf.h"
#include "tf2_msgs/TFMessage.h"
#include <tf/transform_listener.h>
#include <tf2_ros/transform_listener.h>
#include <geometry_msgs/TransformStamped.h>
#include <nav_msgs/Odometry.h>
#include <sensor_msgs/LaserScan.h>
#include <tf2_msgs/TFMessage.h>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2_ros/transform_broadcaster.h>
#include "set_goal/NewGoal.h"
//#include "pick_and_delivery/NewGoal.h"


#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <cstring>  
#include "std_msgs/String.h"
#include <sstream>
#include <deque>
#include <ctime>

#define TIMEOUT 10  //TIMEOUT interazione utente

int num_users=4;

int error=1;
std::string information;

std::vector<float> target_position(2.0); //nuova posizione mentre mi sposto(ad ogni passo)
std::vector<float> old_position(2.0);    //vecchia posizione mentre mi sposto(ad ogni passo)
std::vector<float> current_position(2.0);

geometry_msgs::PoseStamped new_goal_msg;
tf2_ros::Buffer tfBuffer; //buffer per le trasformate
size_t num=10;
int message_published=0;


//se ci stiamo spostando setto a 1 altrimenti 0
int cruising=0;
time_t T=10;
bool DestInAttesa=true;
bool prelevato=false;

ros::Publisher robot;
ros::Publisher pub;
ros::Publisher mitToDest;

//callback usata per vedere la posizione del robot ad ogni istante
	//mi iscrivo al topic tf e chiamare la position_callback.
	//Ogni volta che vengono pubblicati messaggi sul topic tf si avvia la position_callback
    //in modo da avere sempre l' ultima posione del roboto aggiornata
ros::Subscriber sub_tf;


struct user{
	std::string username;
	std::string password;
	float x;
	float y;
	float theta;
};

std::vector<user> users;
std::vector<user> usersLogIn;


struct shipment{
	user mittente;
	user destinatario;
};

struct infoDest{
	int stato;
	std::string infostato;
};


std::deque<shipment> codaTrasporti;

infoDest infoForDest;
bool ReadUsers()
{
		//OSS: bisgona stare nella cartella src per eseguire il server altrimenti non vede il file degli utenti()
		//DA SISTEMARE
	 std::ifstream iFile("./utenti.txt");
	 std::string line;
	 char *ptr;
	 user utente;
	 getline(iFile,line);// elimino la prima riga che è l' intestazione
	 for(int i=0;i<num_users*5;i++)
	 {
		
		/*getline(iFile, line,' ');
		utente.username=line;
		getline(iFile, line,' ');
		utente.password=line;
	    getline(iFile,line,' ');
	    utente.x=stof(line);
	    getline(iFile,line,' ');
	    utente.y=stof(line);
	    getline(iFile,line,' ');
	    utente.theta=stof(line);
		ROS_INFO("%s,%s,%f,%f,%f",utente.username.c_str(),utente.password.c_str(),utente.x,utente.y,utente.theta);
		*/
		/*getline(iFile,line,' ');
		ROS_INFO("%s",line.c_str());*/
		
	  }
	  //DA FAR FUNZIONARE STA MERDA DI LETTURA DA FILE(possibile problema con \n a termine riga)
	  
	  
	  iFile.close();
	  
	  //ABUSIVO
	  utente.username="user1";
	  utente.password="user1";
	  utente.x=53.0;
	  utente.y=18.0;
	  utente.theta=0.02;
	  users.push_back(utente);
	  utente.username="user2";
	  utente.password="user2";
	  utente.x=54.0;
	  utente.y=20.0;
	  utente.theta=0.02;
	  users.push_back(utente);
	  utente.username="user3";
	  utente.password="user3";
	  utente.x=15.0;
	  utente.y=9.0;
	  utente.theta=1.0;
	  users.push_back(utente);
	  utente.username="user4";
	  utente.password="user4";
	  utente.x=10.0;
	  utente.y=10.0;
	  utente.theta=1.0;
	  users.push_back(utente);
	  return true;
}


user getUser(std::string nomeutente)
{
	for (user u:usersLogIn)
	  {
		  if(u.username==nomeutente)
		  {
			  return u;
		  }
	  }
}

bool login_utente(pick_and_delivery::UserLogin::Request  &req, pick_and_delivery::UserLogin::Response &res)
{ 
  for (std::vector<user>::iterator it = users.begin(); it != users.end(); ++it)
  {
	  if(it->username==req.username && it->password==req.password)
	  {
		  user loggato;
		  loggato.username=req.username;
		  loggato.password=req.password;
		  loggato.x=it->x;
		  loggato.y=it->y;
		  loggato.theta=it->theta;
		  res.login="OK";
		  usersLogIn.push_back(loggato);
		  
		  ROS_INFO("user:[%s] ha effettuato il login.",it->username.c_str());
		  return true;
	  }
  }
  res.login="ERROR: Username o password errati";
  ROS_INFO("Tentativo di login dall' user:[%s] non andato a buon fine", req.username.c_str());
  
  return false;
}

bool controllo_send_or_rec_login(pick_and_delivery::ControlSendOrRec::Request  &req, pick_and_delivery::ControlSendOrRec::Response &res)
{
	
	for (user u:usersLogIn)
	  {
		  if(u.username==req.username)
		  {
			  res.responseControl="OK";
			  return true;
		  }
	  }
	res.responseControl="ERRORE: utente mittende/destinatario non loggato sul server";
	return true;
	
}


bool controllo_robot_occupato(pick_and_delivery::ControlRobotReady::Request  &req, pick_and_delivery::ControlRobotReady::Response &res)
{
		shipment trasporto;
		trasporto.mittente=getUser(req.mittente);
		trasporto.destinatario=getUser(req.destinatario);
		//se il mio trasporto non è in coda lo inserisco
		//politica: non più di un trasporto in coda con stesso mittente e destinatario(altrimenti code di priorità)
		std::deque<shipment>::iterator it = codaTrasporti.begin();
		bool presente=false;
		while (it != codaTrasporti.end())
		{  
			  if((*it).mittente.username==req.mittente  && (*it).destinatario.username==req.destinatario)
				{
						presente=true;
						break;
				}
			*it++;
		}  
  
		if(!presente)
			codaTrasporti.push_back(trasporto);
			
		
		//controllo se è il mio turno notifico che tocca a me
		//altrimenti attendo
		
		shipment first=codaTrasporti.front();
		if(first.mittente.username==req.mittente && first.destinatario.username==req.destinatario)
		{
			  res.responseControl="OK";
			  return true;
		}
		
		res.responseControl="ERRORE: trasporto occupato";
	    return true;
	
	
}


void INFO_CallBack(const pick_and_delivery::InfoComunication::ConstPtr& infoRobot)
{
	error=infoRobot->status;
	information=infoRobot->info;
	if(error==1)//successo ovvero il robot è arrivato, mi fermo
		cruising=0;
}
ros::Subscriber subInfo; //callback per le informazioni date da set_goal

/**ROBOT VERSO IL CLIENT MITTENTE*/
bool attesaRobot(pick_and_delivery::Spedizione::Request  &req, pick_and_delivery::Spedizione::Response &res)
{
	
	set_goal::NewGoal set_new_goal_msg;
	user fromTo=getUser(req.fromToUser);
	set_new_goal_msg.x=fromTo.x;
	set_new_goal_msg.y=fromTo.y;
	set_new_goal_msg.theta=fromTo.theta;
	cruising=1;
	pub.publish(set_new_goal_msg);
	ROS_INFO("pubblicata una nuova posizione al robot: x:%f y:%f theta:%f",set_new_goal_msg.x,set_new_goal_msg.y,set_new_goal_msg.theta);

	while(cruising)
	{
		//ROS_INFO("SPOSTAMENTO");
		 ros::spinOnce();
	}
	switch(error)
	{
		case -1: //robot bloccato o non raggiunto in tempo
			res.status=-1;
			res.info=information;
			break;
		/*case 2: //goal non raggiunto in tempo
			res.status=-1;
			res.info="GOAL NON RAGGIUNTO IN TEMPO";
			break;*/
		default:
			res.status=1;
			res.info="ROBOT ARRIVATO";
	}
	return true;
}

/**ROBOT VERSO IL CLIENT DESTINATARIO*/
bool invioRobot(pick_and_delivery::Spedizione::Request  &req, pick_and_delivery::Spedizione::Response &res)
{
	set_goal::NewGoal set_new_goal_msg;
	user fromTo=getUser(req.fromToUser);
	set_new_goal_msg.x=fromTo.x;
	set_new_goal_msg.y=fromTo.y;
	set_new_goal_msg.theta=fromTo.theta;
	cruising=1;
	pub.publish(set_new_goal_msg);
	ROS_INFO("pubblicata una nuova posizione al robot: x:%f y:%f theta:%f",set_new_goal_msg.x,set_new_goal_msg.y,set_new_goal_msg.theta);

	while(cruising)
	{
		//ROS_INFO("SPOSTAMENTO");
		 ros::spinOnce();
	}
	
	//TODO verificare che il destinatario abbia ricevuto e riscontrare al mittente
	
	
	switch(error)
	{
		case -1: //robot bloccato o non raggiunto in tempo
			res.status=-1;
			res.info=information;
			break;
		/*case 2: //goal non raggiunto in tempo
			res.status=-1;
			res.info="GOAL NON RAGGIUNTO IN TEMPO";
			break;*/
		default:
			res.status=1;
			res.info="ROBOT ARRIVATO";
	}
	
	
	infoForDest.stato=res.status;
	infoForDest.infostato=res.info;
	DestInAttesa=false;
	
	//attendo che il destinatario controlli e prelevi il pacco
	while(!prelevato)
	{
		continue;
	}
	
	
	
	
	
	
	return true;
}

/**DESTINATARIO IN ATTESA DEL PACCO*/
bool attesaRobotDestinatario(pick_and_delivery::Spedizione::Request  &req, pick_and_delivery::Spedizione::Response &res)
{
	prelevato=false;
	while(DestInAttesa)
	{
		continue;
	}
	
	//uscito dal ciclo è arrivato il pacco al destinatario
	//notifico al destinatario che il pacco è arrivato 
	res.status=infoForDest.stato;
	res.info=infoForDest.infostato;
	
	
	return true;
}

/**DESTINATARIO RICEVE PACCO */
bool paccoRicevuto(pick_and_delivery::Spedizione::Request  &req, pick_and_delivery::Spedizione::Response &res)
{
	prelevato=true;
	
	//fine spedizione TODO: gestire la coda di spedizione
	
	res.status=1;
	res.info="FINE SPEDIZIONE";
	
	return true;
}








bool SPEDIZIONEFUNZIONE(pick_and_delivery::SpedizioneRobot::Request  &req, pick_and_delivery::SpedizioneRobot::Response &res)
{
	//OSS: nella funzione i res._ sono per il mittente e comunication._ per il destinatario
	ROS_INFO("AVVIATA UNA NUOVA SPEDIZIONE");
	set_goal::NewGoal set_new_goal_msg;
	pick_and_delivery::InfoComunication comunication;
	user mit=getUser(req.mittente);
	user dest=getUser(req.destinatario);

	int tempo;
	set_new_goal_msg.x=mit.x;
	set_new_goal_msg.y=mit.y;
	set_new_goal_msg.theta=mit.theta;
	cruising=1;
	pub.publish(set_new_goal_msg);
	ROS_INFO("pubblicata una nuova posizione al robot: x:%f y:%f theta:%f",set_new_goal_msg.x,set_new_goal_msg.y,set_new_goal_msg.theta);

	while(cruising)
	{
		//ROS_INFO("SPOSTAMENTO");
		 ros::spinOnce();
	}
	switch(error)
	{
		case -1: //robot bloccato o non raggiunto in tempo
			ROS_INFO("errore robot verso il mittente...");
			res.status=-1;
			res.info=information;
			comunication.status=-1;
			comunication.info=information;
			mitToDest.publish(comunication);
			ros::spinOnce();
			return true;
			break;
		default:
			ROS_INFO("robot al mittente");
			res.status=1;
			res.info="ROBOT ARRIVATO";
			comunication.status=1;
			comunication.info="Robot in partenza dal mittente e sta per arrivare...";
			mitToDest.publish(comunication);
			ros::spinOnce();
	}
	ROS_INFO("attendo il pacco...");
	//attendo TIMEOUT secondi che l' utente mittente mette il pacco
	tempo=static_cast<long int> (time(NULL));
	while(static_cast<long int> (time(NULL))<tempo+TIMEOUT)
		continue;
	
	//mando il robot al destinatario
	set_new_goal_msg.x=dest.x;
	set_new_goal_msg.y=dest.y;
	set_new_goal_msg.theta=dest.theta;
	cruising=1;
	pub.publish(set_new_goal_msg);
	ROS_INFO("pubblicata una nuova posizione al robot: x:%f y:%f theta:%f",set_new_goal_msg.x,set_new_goal_msg.y,set_new_goal_msg.theta);

	while(cruising)
	{
		//ROS_INFO("SPOSTAMENTO");
		 ros::spinOnce();
	}
	
	switch(error)
	{
		case -1: //robot bloccato o non raggiunto in tempo
			ROS_INFO("errore robot verso il destinatario...");
			res.status=-1;
			res.info=information;
			comunication.status=-1;
			comunication.info=information;
			mitToDest.publish(comunication);
			ros::spinOnce();
			return true;
			break;
		default:
			ROS_INFO("robot al destinatario");
			res.status=1;
			res.info="ROBOT ARRIVATO";
	}
	
	//publico un messaggio al destinatario che il robot è arrivato
	comunication.status=1;
	comunication.info="ROBOT ARRIVATO";
	mitToDest.publish(comunication);
	ros::spinOnce();
	//attendo che il destintario prelevi il pacco
	ROS_INFO("attendo che il destinatario prelevi il pacco...");
	tempo=static_cast<long int> (time(NULL));
	while(static_cast<long int> (time(NULL))<tempo+TIMEOUT)
		continue;
	
	
	//esaurisco la spedizione
	return true;
}







int main(int argc, char **argv)
{
  ros::init(argc, argv, "pick_and_delivery");
  
  ros::NodeHandle n;
    
  ROS_INFO("SERVER RUN");
  //leggo gli utenti registrati al mio servizio
  if(!ReadUsers()) return 1;
  
  ROS_INFO("Utenti caricati sul server. Presenti %d utenti registrati al servizio",static_cast<int>(users.size()));
  for(user u:users)
	  ROS_INFO("user: %s",u.username.c_str());
  
  //per pubblicare un messaggio di tipo new goal
  pub=n.advertise<set_goal::NewGoal>("New_Goal",1000);
  subInfo=n.subscribe("infoComunication",1000,INFO_CallBack);
  mitToDest=n.advertise<pick_and_delivery::InfoComunication>("mitToDest",1000);
	
  //servizio ROS
  ros::ServiceServer service = n.advertiseService("UserLogin", login_utente);
  ros::ServiceServer service_ControlSendOrRec=n.advertiseService("ControlSendOrRec",controllo_send_or_rec_login);
  ros::ServiceServer service_ControlRobotReady=n.advertiseService("ControlRobotReady",controllo_robot_occupato); //da testare per bene
  ros::ServiceServer service_AttesaRobot=n.advertiseService("AttesaRobot",attesaRobot);
  ros::ServiceServer service_InvioRobot=n.advertiseService("InvioRobot",invioRobot);
  ros::ServiceServer service_AttesaRobotDestinatario=n.advertiseService("AttesaRobotDestinatario",attesaRobotDestinatario);
  ros::ServiceServer service_PaccoRicevuto=n.advertiseService("PaccoRicevuto",paccoRicevuto);
  ros::ServiceServer service_SPEDIZIONE=n.advertiseService("SPEDIZIONE",SPEDIZIONEFUNZIONE);

  ROS_INFO("SERVER READY TO ACCEPT REQUEST");
  
  ros::spin();

  return 0;
}
