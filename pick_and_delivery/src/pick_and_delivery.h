#include <string>
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <cstring>  
struct user{
	std::string username;
	std::string password;
	float x;
	float y;
	float theta;
};

bool ReadUsers(std::vector<user> users)
{
	int num_users=4;
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
	  //DA FAR FUNZIONARE STA MERDA DI LETTURA DA FILE
	  
	  
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
