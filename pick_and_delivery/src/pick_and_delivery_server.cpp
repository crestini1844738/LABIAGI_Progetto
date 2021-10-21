#include "ros/ros.h"
#include "pick_and_delivery/UserLogin.h"
#include "pick_and_delivery/ControlSendOrRec.h"
#include "pick_and_delivery/ControlRobotReady.h"
#include "pick_and_delivery/SpedizioneRobot.h"

#include "pick_and_delivery/InfoComunication.h" //due campi: status e info
												//status 0: ancora in esecuzione
												//status 1: terminato con successo; info con messaggio di successo
												//status -1: terminato con errore; info errore
#include "set_goal/NewGoal.h"


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
#include <numeric>
#include <string_view>
#define TIMEOUT 10  //TIMEOUT interazione utente

int num_users=4;

int error=1;
std::string information;

//std::vector<float> target_position(2.0); //nuova posizione mentre mi sposto(ad ogni passo)
//std::vector<float> old_position(2.0);    //vecchia posizione mentre mi sposto(ad ogni passo)
//std::vector<float> current_position(2.0);


size_t num=10;
int message_published=0;


//se ci stiamo spostando setto a 1 altrimenti 0
int cruising=0;
time_t T=10;
bool DestInAttesa=true;
bool prelevato=false;

//ros::Publisher robot;
ros::Publisher pub;
ros::Publisher mitToDest;




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
	
	  user utente;
	  utente.username="user1";
	  utente.password="user1";
	  utente.x=53.0; //53
	  utente.y=18.0; //18
	  utente.theta=0.02;
	  users.push_back(utente);
	  utente.username="user2";
	  utente.password="user2";
	  utente.x=51.5;
	  utente.y=7.0;
	  utente.theta=0.02;
	  users.push_back(utente);
	  utente.username="user3";
	  utente.password="user3";
	  utente.x=32.5;
	  utente.y=12.0;
	  utente.theta=0.02;
	  users.push_back(utente);
	  //potevo leggere gli utenti da un file(utenti.txt)
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
	if(error!=0)//successo ovvero il robot è arrivato, mi fermo
		cruising=0;
}
ros::Subscriber subInfo; //callback per le informazioni date da set_goal




void fine_Spedizione(user mittente,user destinatario)
{
	//estraggo dalla coda la spedizione
	//eseguo il logout degli utenti che hanno fatto la spedizione
	
	codaTrasporti.pop_front(); //elimino il primo in coda
	ROS_INFO("user:[%s] ha effettuato il logout.",mittente.username.c_str());
	ROS_INFO("user:[%s] ha effettuato il logout.",destinatario.username.c_str());
	std::vector<user> new_usersLogIn;	
	for (user u:usersLogIn)
	  {
		  if(u.username!=mittente.username && u.username!=destinatario.username)
		  {
			  new_usersLogIn.push_back(u);
		  }
	  }
	
	
	usersLogIn=new_usersLogIn;
	
	ROS_INFO("num spedizioni in coda: %d",static_cast<int>(codaTrasporti.size()));
	ROS_INFO("utenti loggati:%d",static_cast<int>(usersLogIn.size()));

}



bool SPEDIZIONEFUNZIONE(pick_and_delivery::SpedizioneRobot::Request  &req, pick_and_delivery::SpedizioneRobot::Response &res)
{
	//OSS: nella funzione i res._ sono per il mittente e comunication._ per il destinatario
	ROS_INFO("AVVIATA UNA NUOVA SPEDIZIONE");
	set_goal::NewGoal set_new_goal_msg;
	pick_and_delivery::InfoComunication comunication;
	user mit=getUser(req.mittente);
	user dest=getUser(req.destinatario);
	ros::Rate loop_rate(T);
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
		 loop_rate.sleep();
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
			fine_Spedizione(getUser(req.mittente),getUser(req.destinatario));
			return true;
			break;
		default:
			ROS_INFO("robot al mittente");
			res.status=1;
			res.info="ROBOT ARRIVATO";
			comunication.status=2;
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
	set_goal::NewGoal set_new_goal_msg_dest;

	set_new_goal_msg_dest.x=dest.x;
	set_new_goal_msg_dest.y=dest.y;
	set_new_goal_msg_dest.theta=dest.theta;
	cruising=1;
	pub.publish(set_new_goal_msg_dest);
	ROS_INFO("pubblicata una nuova posizione al robot: x:%f y:%f theta:%f",set_new_goal_msg_dest.x,set_new_goal_msg_dest.y,set_new_goal_msg_dest.theta);

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
			fine_Spedizione(getUser(req.mittente),getUser(req.destinatario));
			return true;
			break;
		default:
			ROS_INFO("robot al destinatario");
			res.status=1;
			res.info="ROBOT ARRIVATO";
	}
	
	//pubblico un messaggio al destinatario che il robot è arrivato
	comunication.status=1;
	comunication.info="ROBOT ARRIVATO";
	mitToDest.publish(comunication);
	ros::spinOnce();
	//attendo che il destintario prelevi il pacco
	ROS_INFO("attendo che il destinatario prelevi il pacco...");
	tempo=static_cast<long int> (time(NULL));
	while(static_cast<long int> (time(NULL))<tempo+TIMEOUT)
		continue;
	
	ROS_INFO("TERMINATA SPEDIZIONE");
	//esaurisco la spedizione
	fine_Spedizione(getUser(req.mittente),getUser(req.destinatario));
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
  ros::ServiceServer service_SPEDIZIONE=n.advertiseService("SPEDIZIONE",SPEDIZIONEFUNZIONE);

  ROS_INFO("SERVER READY TO ACCEPT REQUEST");
  
  ros::spin();

  return 0;
}
