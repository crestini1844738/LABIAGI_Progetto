#include "ros/ros.h"
#include "pick_and_delivery/UserLogin.h"
#include "pick_and_delivery/ControlSendOrRec.h"

//#include "pick_and_delivery.h"
#include <string>
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <cstring>  
#include "std_msgs/String.h"
#include <sstream>
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
	  
	  return true;
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
  
  //servizio ROS
  ros::ServiceServer service = n.advertiseService("UserLogin", login_utente);
  ros::ServiceServer service_ControlSendOrRec=n.advertiseService("ControlSendOrRec",controllo_send_or_rec_login);
  ROS_INFO("SERVER READY TO ACCEPT REQUEST");
  
  ros::spin();

  return 0;
}
