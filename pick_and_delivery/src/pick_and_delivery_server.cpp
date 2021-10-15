#include "ros/ros.h"
#include "pick_and_delivery/UserLogin.h"
#include "pick_and_delivery/ControlSendOrRec.h"
#include "pick_and_delivery/ControlRobotReady.h"
#include "pick_and_delivery/Spedizione.h"
#include "pick_and_delivery/InfoComunication.h" //due campi: status e info
												//status 0: ancora in esecuzione
												//status 1: terminato con successo; info con messaggio di successo
												//status -1: terminato con errore; info errore

//#include "pick_and_delivery.h"
#include <string>
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <cstring>  
#include "std_msgs/String.h"
#include <sstream>
#include <deque>
int num_users=4;



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

std::deque<shipment> codaTrasporti;

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
	  utente.x=10.0;
	  utente.y=10.0;
	  utente.theta=0.0;
	  users.push_back(utente);
	  utente.username="user2";
	  utente.password="user2";
	  utente.x=20.0;
	  utente.y=20.0;
	  utente.theta=0.0;
	  users.push_back(utente);
	  utente.username="user3";
	  utente.password="user3";
	  utente.x=30.0;
	  utente.y=30.0;
	  utente.theta=0.0;
	  users.push_back(utente);
	  utente.username="user4";
	  utente.password="user4";
	  utente.x=40.0;
	  utente.y=40.0;
	  utente.theta=0.0;
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

 ros::Publisher server_to_clientMittente;
 
bool spedizione(pick_and_delivery::Spedizione::Request  &req, pick_and_delivery::Spedizione::Response &res)
{
	pick_and_delivery::InfoComunication message;
	message.status=0;
	message.info="arrivato";
	server_to_clientMittente.publish(message);
	ros :: spinOnce (); 
	ROS_INFO("ARRIVATO");
	return true;
}



int main(int argc, char **argv)
{
  ros::init(argc, argv, "pick_and_delivery");
  ros::NodeHandle n;

  server_to_clientMittente = n.advertise<pick_and_delivery::InfoComunication>("server_to_clientMittente", 1000);
  
  ROS_INFO("SERVER RUN");
  //leggo gli utenti registrati al mio servizio
  if(!ReadUsers()) return 1;
  
  ROS_INFO("Utenti caricati sul server. Presenti %d utenti registrati al servizio",static_cast<int>(users.size()));
  for(user u:users)
	  ROS_INFO("user: %s",u.username.c_str());
  
  //servizio ROS
  ros::ServiceServer service = n.advertiseService("UserLogin", login_utente);
  ros::ServiceServer service_ControlSendOrRec=n.advertiseService("ControlSendOrRec",controllo_send_or_rec_login);
  ros::ServiceServer service_ControlRobotReady=n.advertiseService("ControlRobotReady",controllo_robot_occupato); //da testare per bene
  ros::ServiceServer service_Spedizione=n.advertiseService("Spedizione",spedizione);

  ROS_INFO("SERVER READY TO ACCEPT REQUEST");
  
  ros::spin();

  return 0;
}
