#include "ros/ros.h"
#include "pick_and_delivery/UserLogin.h"
#include "pick_and_delivery/ControlSendOrRec.h"
#include "pick_and_delivery/ControlRobotReady.h"
#include "pick_and_delivery/Spedizione.h"
#include "pick_and_delivery/InfoComunication.h"
#include "pick_and_delivery/SpedizioneRobot.h"


#include <cstdlib>
#include "std_msgs/String.h"
#include <sstream>
#include <ctime>
#define TIMEOUT 10  //TIMEOUT interazione utente
#define TIMEOUT2 10  //TIMEOUT di attesa del robot impegnato per altri trasporti

std::string c;
std::string my_user;
std::string my_pass;
std::string scelta; //send per inviare,rec  per ricevere
std::string other_user; //definisce l' altro utente a cui voglio spedire o da cui voglio ricevere
std::string inputString;
int controllo_timer=0;
bool arrivato=false;
int attesa_interazione_utente;
int tempo=0;
ros::Subscriber sub_info_spedizione;



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









void ExitFailTimeout(std::string errore)
{
		//gestire logout e coda spedizioni
		ROS_ERROR("TIMEOUT! %s",errore.c_str());	
		exit(EXIT_FAILURE);	
}

void ExitFail(std::string errore)
{
	    //gestire logout e coda spedizioni
		ROS_ERROR("%s",errore.c_str());	
		exit(EXIT_FAILURE);	
}

void ExitSuccess(std::string errore)
{
	    //gestire logout e coda spedizioni
		ROS_INFO("SPEDIZIONE TERMINATA");	
		exit(EXIT_SUCCESS);	
}

void check_CallBackUtente(const ros::TimerEvent& event)
{
	//se sto aspettando che l' utente metta il pacco o lo prelevi
	if(attesa_interazione_utente!=0)
	{
		attesa_interazione_utente=0;
		//TODO: Da gestire sul server... fare una servizio sul server per annullare la spedizione
		ExitFailTimeout("Tempo scaduto per interagire con il robot");
		
	}
}



void mitToDest_CallBack(const pick_and_delivery::InfoComunication::ConstPtr& infoRobot)
{
	int error=infoRobot->status;
	std::string information=infoRobot->info;
	if(error==-1)//successo ovvero il robot è arrivato, mi fermo
		ExitFail(information);
	ExitSuccess(information);
}


/**SPEDIZIONE IN USCITA, CLIENT INVIA IL PACCO*/
void client_mittente(ros::ServiceClient client, ros::NodeHandle n) 
{
	
	ROS_INFO("SPEDIZIONE PACCO");
	client = n.serviceClient<pick_and_delivery::SpedizioneRobot>("SPEDIZIONE");
    pick_and_delivery::SpedizioneRobot srvSpedizioneRobot;
    srvSpedizioneRobot.request.mittente = my_user;
    srvSpedizioneRobot.request.destinatario = other_user; 
	 ROS_INFO("Spedizione avviata...");
	  if (client.call(srvSpedizioneRobot))
	  {
			if(srvSpedizioneRobot.response.status==-1)
				ExitFail(srvSpedizioneRobot.response.info);
			else
			    ExitSuccess(srvSpedizioneRobot.response.info);
		 
		
	  }
	  else
	  {
		ExitFail("Failed to call service AttesaRobot");
	  }
  /*
  client = n.serviceClient<pick_and_delivery::Spedizione>("AttesaRobot");
  pick_and_delivery::Spedizione srvAttesaRobot;
  srvAttesaRobot.request.fromToUser = my_user; //il robot viene prima da me
  
  ROS_INFO("Spedizione avviata...");
  if (client.call(srvAttesaRobot))
  {
	    if(srvAttesaRobot.response.status==-1)
			ExitFail(srvAttesaRobot.response.info);
	 
	
  }
  else
  {
	ExitFail("Failed to call service AttesaRobot");
  }
  
  ROS_INFO("ROBOT ARRIVATO");
  attesa_interazione_utente=1;
  while(true)
  {
    ROS_INFO("Metti il pacco sul robot!! Inserisci 'ok' per continuare ");
    std::getline(std::cin, c);
    if(c!="ok")
      ROS_INFO("comando non riconosciuto");
    else
		break;
  }
   attesa_interazione_utente=0;
   ROS_INFO("ROBOT VERSO IL DESTINATARIO");
   client = n.serviceClient<pick_and_delivery::Spedizione>("InvioRobot");
   pick_and_delivery::Spedizione srvInvioRobot;
   srvInvioRobot.request.fromToUser = other_user; //il robot va al destinatario
   ROS_INFO("In attesa di riscontro...");
   
   
   if (client.call(srvInvioRobot))
  {
	    if(srvInvioRobot.response.status==-1)
			ExitFail(srvInvioRobot.response.info);
  }
  else
  {
	ExitFail("Failed to call service InvioRobot");
  }
  ROS_INFO("PACCO RICEVUTO.");*/
}

/**SPEDIZIONE IN INGRESSO, CLIENT RICEVE IL PACCO*/

void client_destinatario(ros::ServiceClient client,ros::NodeHandle n)
{
	ROS_INFO("RICEZIONE PACCO");
	sub_info_spedizione=n.subscribe("mitToDest",1000,mitToDest_CallBack);

	ros::spin();
	/** attendo che il robot arrivi alla mia posizione*/
	/*ROS_INFO("In attesa di arrivo del pacco...");

    client = n.serviceClient<pick_and_delivery::Spedizione>("AttesaRobotDestinatario");
	pick_and_delivery::Spedizione srvAttesaRobotDestinatario;
	srvAttesaRobotDestinatario.request.fromToUser = other_user; //ricevo da questo utente
	if (client.call(srvAttesaRobotDestinatario))
	  {
			if(srvAttesaRobotDestinatario.response.status==-1)
				ExitFail(srvAttesaRobotDestinatario.response.info);
		 
		
	  }
	  else
	  {
		ExitFail("Failed to call service AttesaRobot");
	  }
  ROS_INFO("ROBOT ARRIVATO");
  attesa_interazione_utente=1;
  while(true)
  {
    ROS_INFO("Preleva il pacco!! Inserisci 'ok' per continuare ");
    std::getline(std::cin, c);
    if(c!="ok")
      ROS_INFO("comando non riconosciuto");
    else
		break;
  }
   attesa_interazione_utente=0;
   //notifico al server che il destinatario ha prelevato il pacco
   client = n.serviceClient<pick_and_delivery::Spedizione>("PaccoRicevuto");
	pick_and_delivery::Spedizione srvPaccoRicevuto;
	srvPaccoRicevuto.request.fromToUser = my_user; //sono l' utente che ha ricevuto
	if (client.call(srvPaccoRicevuto))
	  {
			if(srvPaccoRicevuto.response.status==-1)
				ExitFail(srvPaccoRicevuto.response.info);
		 
		
	  }
	  else
	  {
		ExitFail("Failed to call service AttesaRobot");
	  }*/
}






int main(int argc, char **argv)
{
  /** Controllo argomenti passati[user,password]*/
  if (argc != 3)
  {
    ROS_INFO("[input non valido] user, password ");
    return 1;
  }
  /**inizializzazione*/
  std::string client_name="pick_and_delivery_client_";
  std::string client_name2=argv[1];
  ros::init(argc, argv, client_name+client_name2);
  ros::NodeHandle n;
  
  /**timer Timeout utente*/
  ros::Timer timerUtente=n.createTimer(ros::Duration(50),check_CallBackUtente);
  
  /**servizio login*/
  ros::ServiceClient client = n.serviceClient<pick_and_delivery::UserLogin>("UserLogin");
  pick_and_delivery::UserLogin srvUserLogin;
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

  //invio le specifiche del mio trasporto cioè chi è il mittente e chi il destinatario
  //nel server se nella coda dei trasporti il primo in coda è proprio il mio trasporto posso procedere
  //altrimenti attendo il mio turno
  //se l' attesa è troppo elevata esco(TIMEOUT2) OSS:posso scegliere di attendere all' infinito finche non è il mio turno
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

  
  
 /**avvio le funzioni specifiche per mittente e destintario*/  
  if(scelta.compare("send")==0)
  
    client_mittente(client,n);
  else
      client_destinatario(client,n);
  
  return 0;
}
