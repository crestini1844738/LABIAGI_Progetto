#include "ros/ros.h"
#include "pick_and_delivery/UserLogin.h"
#include "pick_and_delivery/ControlSendOrRec.h"
#include "pick_and_delivery/ControlRobotReady.h"

#include <cstdlib>
#include "std_msgs/String.h"
#include <sstream>
#include <ctime>
#define TIMEOUT 10  //TIMEOUT interazione utente
#define TIMEOUT2 10  //TIMEOUT di attesa del robot impegnato per altri trasporti

std::string my_user;
std::string my_pass;
std::string scelta; //send per inviare,rec  per ricevere
std::string other_user; //definisce l' altro utente a cui voglio spedire o da cui voglio ricevere
std::string inputString;
int controllo_timer=0;
//int TIMEOUT=10;
int tempo=0;
void send_or_receive() //scelta se mittente e destinatario e altro utente
{
	while(true)
  {
	//std::cout << "Give input";
	ROS_INFO("Vuoi inviare o ricevere un pacco? (send or rec)");
    std::getline(std::cin, scelta);
    if(scelta.compare("send") != 0 && scelta.compare("rec") != 0 )
      ROS_INFO("Scelta non valida(send or rec)");
    else
		break;
  }
  while(true)
  {
    ROS_INFO("Inserisci l' Username del mittente/destinatario:");
    std::getline(std::cin, other_user);
    if(other_user==my_user)
      ROS_INFO("Non puoi essere sia mittente che destinatario");
    else
		break;
  }
}
void client_mittente(ros::NodeHandle n) //per gestire un client che vuole INVIARE un pacco
{
	ROS_INFO("SPEDIZIONE PACCO");
}



void client_destinatario(ros::NodeHandle n) //per gestire un client che vuole RICEVERE un pacco
{
	ROS_INFO("RICEZIONE PACCO");
}

void ExitFailTimeout(std::string errore)
{
		ROS_ERROR("TIMEOUT! %s",errore.c_str());	
		exit(EXIT_FAILURE);	
}

int main(int argc, char **argv)
{
  
  /** Controllo argomenti passati[user,password]*/
  ros::init(argc, argv, "pick_and_delivery_client");
  if (argc != 3)
  {
    ROS_INFO("[input non valido] user, password ");
    return 1;
  }

  ros::NodeHandle n;

  /**servizio login*/
  ros::ServiceClient client = n.serviceClient<pick_and_delivery::UserLogin>("UserLogin");
  pick_and_delivery::UserLogin srvUserLogin;
  
  //prelevo utente e password da argv e richiedo il login al server
  my_user=argv[1];
  my_pass=argv[2];
  srvUserLogin.request.username = argv[1];
  srvUserLogin.request.password = argv[2];
  if (client.call(srvUserLogin))
  {
	if(srvUserLogin.response.login!="OK")
	{	ROS_ERROR("%s",srvUserLogin.response.login.c_str());
		return 1;
	}
  }
  else
  {
	ROS_ERROR("Failed to call service UserLogin");
	return 1;
  }
  
  ROS_INFO("SERVER RESPONSE: LOGIN %s",srvUserLogin.response.login.c_str());
  
  
  
  /**scelta client di invio o ricezione pacco  e utente mittente/destinatario*/
  send_or_receive();
  
 
  /**controllo che l' altro utente sia loggato*/
  ROS_INFO("In attesa che il mittente/destinatario sia connesso al server...");
  
  
  client=n.serviceClient<pick_and_delivery::ControlSendOrRec>("ControlSendOrRec");
  pick_and_delivery::ControlSendOrRec srvControlSendOrRec;
  
  srvControlSendOrRec.request.username=other_user;
  
  controllo_timer=1;
  tempo=static_cast<long int> (time(NULL));
  while(static_cast<long int> (time(NULL))<tempo+TIMEOUT)
  {
	  if (client.call(srvControlSendOrRec))
	  {
			if(srvControlSendOrRec.response.responseControl!="OK")
			{	
				continue;
			}
			else
			{
				controllo_timer=0;
				break;
			}
	  }
	  else
	  {
			ROS_ERROR("Failed to call service ControlSendOrRec");
			return 1;
	  }
  }
  if(controllo_timer) ExitFailTimeout("utente non loggato in tempo");
  ROS_INFO("OK. Altro utente connesso!");


/**CONTROLLO che il robot non sia già impegnato in altri trasporti*/

  ROS_INFO("In attesa che il robot sia disponibile...");
  client=n.serviceClient<pick_and_delivery::ControlRobotReady>("ControlRobotReady");
  pick_and_delivery::ControlRobotReady srvControlRobotReady;
  
  if(scelta.compare("send")==0)
  {
	  //se sono io che invio
	  srvControlRobotReady.request.mittente=my_user; //mittente sono io
      srvControlRobotReady.request.destinatario=other_user; //destinatario è l' altro
  }
  else
  {
	  //se sono io che ricevo
	  srvControlRobotReady.request.mittente=other_user; //mittente è l' altro
      srvControlRobotReady.request.destinatario=my_user; //destinatario sono io
  }
  




  controllo_timer=1;
  tempo=static_cast<long int> (time(NULL));
  while(static_cast<long int> (time(NULL))<tempo+TIMEOUT2)
  {
	  if (client.call(srvControlRobotReady))
	  {
			if(srvControlRobotReady.response.responseControl!="OK")
			{	
				continue;
			}
			else
			{
				controllo_timer=0;
				break;
			}
	  }
	  else
	  {
			ROS_ERROR("Failed to call service ControlRobotReady");
			return 1;
	  }
  }
  if(controllo_timer) ExitFailTimeout("tempo di attesa del robot elevato");
  ROS_INFO("OK! Robot pronto per la spedizione");




//TODO
  /**iniziare l'invio/ricezione*/
   //start per iniziare la connessione(OSS:il destinatario non deve fare questo start)
   while(true)
  {
	ROS_INFO("S to start");
    std::getline(std::cin, inputString);
    if(inputString.compare("S") != 0 )
      ROS_INFO("Scelta non valida(send or rec)");
    else
		break;
  }
  
  
  
  
  
  
  
  
  
  if(scelta.compare("send")==0)
	  client_mittente(n);
  else
      client_destinatario(n);
  
  
  
	  
  return 0;
}
