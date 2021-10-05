#include "ros/ros.h"
#include "pick_and_delivery/UserLogin.h"
#include <cstdlib>

std::string scelta; //send per inviare,rec  per ricevere
std::string other_user; //definisce l' altro utente a cui voglio spedire o da cui voglio ricevere

void send_or_receive() //scelta di mittente e destinatario
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
	
}
void client_mittente() //per gestire un client che vuole INVIARE un pacco
{
	ROS_INFO("SPEZIONE PACCO");
}



void client_destinatario() //per gestire un client che vuole RICEVERE un pacco
{
	ROS_INFO("RICEZIONE PACCO");
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
  srvUserLogin.request.username = argv[1];
  srvUserLogin.request.password = argv[2];
  if (client.call(srvUserLogin))
  {
	if(srvUserLogin.response.login=="ERROR")
	{	ROS_ERROR("ERROR LOGIN!");
		return 1;
	}
  }
  else
  {
	ROS_ERROR("Failed to call service UserLogin");
	return 1;
  }
  
  ROS_INFO("SERVER RESPONSE: LOGIN %s",srvUserLogin.response.login.c_str());
  
  
  
  /**scelta client di invio o ricezione pacco  */
  send_or_receive();
  
  
  if(scelta.compare("send"))
	  client_mittente();
  else
      client_destinatario();
  
  
  
	  
  return 0;
}
