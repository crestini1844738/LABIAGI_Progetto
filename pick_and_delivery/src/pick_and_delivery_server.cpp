#include "ros/ros.h"
#include "pick_and_delivery/UserLogin.h"
#include "pick_and_delivery/ControlSendOrRec.h"
#include "pick_and_delivery/ControlRobotReady.h"
#include "pick_and_delivery/Spedizione.h"
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

int num_users=4;

int error=0;


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

ros::Publisher robot;

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

//callback che ci dice quale è la posizione del robot in quell istante
void position_CallBack(const tf2_msgs::TFMessage& tf)
{
	int transform_ok=tfBuffer.canTransform("map","base_link",ros::Time(0));
	if(transform_ok!=0)//posso trasformare 
	{
		geometry_msgs::TransformStamped transformStamped;
		transformStamped = tfBuffer.lookupTransform("map","base_link",ros::Time(0));
		ROS_INFO("trasformata:[x:%f , y:%f]",transformStamped.transform.translation.x,transformStamped.transform.translation.y);
		//è sufficente controllare le x e le y per vedere se mi sto muovendo
		//theta non serve. se lo metto ho problemi di gestione di altro genere
		current_position[0]=transformStamped.transform.translation.x;
		current_position[1]=transformStamped.transform.translation.y;

	}
}


//callback con timer. Prende un messaggio di tipo timer
//per vedere se effetivamente il robot si sta spostando
//viene chiamata molto frequentemente
void check1_CallBack(const ros::TimerEvent& event)
{
	//se sono in movimento
	if(cruising!=0)
	{
		ROS_INFO("Controllo se mi sto muovendo...");
		
		//prendo la distanza tra la posizione corrente e quella precendete.
		//se minore di un certo valore significa che mi sono bloccato
		float distance;
		distance=sqrt(pow(current_position[0]-old_position[0],2)+pow(current_position[1]-old_position[1],2));
		if(distance < 0.8)
		{
			ROS_INFO("PROBLEMAAA!!!");
			error=1;
			//cruising=0;
		}
		
		//per verificare se sono arrivato al goal
		//controllo tra la posizione corrente e la posizione target
		//se sono arrivato termino di muovermi(cruising=0)
		if(sqrt(pow(current_position[0]-target_position[0],2)+pow(current_position[1]-target_position[1],2)) < 1.5)
		{
			ROS_INFO("Arrivato al goal");
			error=0;
			cruising=0; //non sono più in movimento
		}
	}
}


//callback per vedere se è passato troppo tempo per arrivare al goal
//viene chiamata solo dopo un tot di tempo
void check2_CallBack(const ros::TimerEvent& event)
{
	//se mi sto muovendo
	if(cruising!=0)
	{
		ROS_INFO("Controllo se è passato troppo tempo...");
		float distance;
		distance=sqrt(pow(current_position[0]-target_position[0],2)+pow(current_position[1]-target_position[1],2));
		if(distance> 0.5)
		{
			ROS_INFO("TIMEOUT: Goal non raggiunto in tempo");
			error=2;
			cruising=0;
		}
	}
}


/**ROBOT VERSO IL CLIENT MITTENTE*/
bool attesaRobot(pick_and_delivery::Spedizione::Request  &req, pick_and_delivery::Spedizione::Response &res)
{
	//prelevo dalla coda dei trasporti il primo trasporto in coda con mittente e destinatario che corrispondono a Request.mittente e Request.destinatario
	//shipment trasporto=codaTrasporti.front();
	user fromTo=getUser(req.fromToUser);
	
	/**setto la posizione di dove andare  prelevare il pacco*/
	new_goal_msg.header.seq=num;
	num++;
	new_goal_msg.header.stamp=ros::Time::now();
	new_goal_msg.header.frame_id="map";
	new_goal_msg.pose.position.x=fromTo.x;
	new_goal_msg.pose.position.y=fromTo.y;
	new_goal_msg.pose.position.z=0;
	new_goal_msg.pose.orientation.x=0;
	new_goal_msg.pose.orientation.y=0;
	new_goal_msg.pose.orientation.z=0;
	new_goal_msg.pose.orientation.w=fromTo.theta;
	
	//per fare in modo che la callback viene chiamata solo una volta
	message_published=1;
	
	//mi sto spostando
	cruising=1;
	
	
	//salvare la goal position
	target_position[0]=new_goal_msg.pose.position.x;
	target_position[1]=new_goal_msg.pose.position.y;
	
	//pubblico la nuova posizione che il robot andrà ad assumere
	ROS_INFO("pubblicata una nuova posizione al robot: x:%f y:%f theta:%f",new_goal_msg.pose.position.x,new_goal_msg.pose.position.y,new_goal_msg.pose.orientation.w);
	
	tf2_ros::TransformListener tfListener(tfBuffer);
	ros::Rate loop_rate(T);
	
	while(true)
	{
		if(message_published!=0)
		{
			ROS_INFO("Publishing a new goal position");
			robot.publish(new_goal_msg);
			message_published=0;
		}
		
		ros::spinOnce();
		loop_rate.sleep();
		
		if(!cruising)
			break;
		
	}
	switch(error)
	{
		case 1: //robot bloccato
			res.status=-1;
			res.info="ROBOT BLOCCATO";
			break;
		case 2: //goal non raggiunto in tempo
			res.status=-1;
			res.info="GOAL NON RAGGIUNTO IN TEMPO";
			break;
		default:
			res.status=1;
			res.info="ROBOT ARRIVATO AL MITTENTE";
	}
	
	return true;
}

/**ROBOT VERSO IL CLIENT DESTINATARIO*/
bool invioRobot(pick_and_delivery::Spedizione::Request  &req, pick_and_delivery::Spedizione::Response &res)
{
	//prelevo dalla coda dei trasporti il primo trasporto in coda con mittente e destinatario che corrispondono a Request.mittente e Request.destinatario
	shipment trasporto=codaTrasporti.front();
	user mittente=trasporto.mittente;
	user destinatario=trasporto.destinatario;
	/**setto la posizione di dove andare a portare il pacco*/
	new_goal_msg.header.seq=num;
	num++;
	new_goal_msg.header.stamp=ros::Time::now();
	new_goal_msg.header.frame_id="map";
	new_goal_msg.pose.position.x=32;
	new_goal_msg.pose.position.y=10;
	new_goal_msg.pose.position.z=0;
	new_goal_msg.pose.orientation.x=0;
	new_goal_msg.pose.orientation.y=0;
	new_goal_msg.pose.orientation.z=0;
	new_goal_msg.pose.orientation.w=0.02;
	
	//per fare in modo che la callback viene chiamata solo una volta
	message_published=1;
	
	//mi sto spostando
	cruising=1;
	
	
	//salvare la goal position
	target_position[0]=new_goal_msg.pose.position.x;
	target_position[1]=new_goal_msg.pose.position.y;
	
	//pubblico la nuova posizione che il robot andrà ad assumere
	ROS_INFO("pubblicata una nuova posizione al robot: x:%f y:%f theta:%f",new_goal_msg.pose.position.x,new_goal_msg.pose.position.y,new_goal_msg.pose.orientation.w);
	
	//tf2_ros::TransformListener tfListener(tfBuffer);
	ros::Rate loop_rate(T);
	
	while(true)
	{
		if(message_published!=0)
		{
			ROS_INFO("Publishing a new goal position");
			robot.publish(new_goal_msg);
			message_published=0;
		}
		
		ros::spinOnce();
		loop_rate.sleep();
		
		if(!cruising)
			break;
		
	}
	switch(error)
	{
		case 1: //robot bloccato
			res.status=-1;
			res.info="ROBOT BLOCCATO";
			break;
		case 2: //goal non raggiunto in tempo
			res.status=-1;
			res.info="GOAL NON RAGGIUNTO IN TEMPO";
			break;
		default:
			res.status=1;
			res.info="ROBOT ARRIVATO AL DESTINATARIO";
	}
	
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
  //tf2_ros::TransformListener tfListener(tfBuffer);

  robot=n.advertise<geometry_msgs::PoseStamped>("/move_base_simple/goal",1000);
  sub_tf=n.subscribe("tf",1000,position_CallBack);
  
  
  //settare callback con un certo periodo di tempo
	ros::Timer timer1=n.createTimer(ros::Duration(0.5),check1_CallBack);
	ros::Timer timer2=n.createTimer(ros::Duration(50),check2_CallBack);
	
	
  //servizio ROS
  ros::ServiceServer service = n.advertiseService("UserLogin", login_utente);
  ros::ServiceServer service_ControlSendOrRec=n.advertiseService("ControlSendOrRec",controllo_send_or_rec_login);
  ros::ServiceServer service_ControlRobotReady=n.advertiseService("ControlRobotReady",controllo_robot_occupato); //da testare per bene
  ros::ServiceServer service_AttesaRobot=n.advertiseService("AttesaRobot",attesaRobot);
  ros::ServiceServer service_InvioRobot=n.advertiseService("InvioRobot",invioRobot);

  ROS_INFO("SERVER READY TO ACCEPT REQUEST");
  
  ros::spin();

  return 0;
}
