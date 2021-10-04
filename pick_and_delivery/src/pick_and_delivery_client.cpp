#include "ros/ros.h"
#include "pick_and_delivery/UserLogin.h"
#include <cstdlib>

int main(int argc, char **argv)
{

  ros::init(argc, argv, "pick_and_delivery_client");
  if (argc != 4)
  {
    ROS_INFO("[input non valido] user, password , s || r (send_or_receive_box)");
    return 1;
  }

  ros::NodeHandle n;
  ros::ServiceClient client = n.serviceClient<pick_and_delivery::UserLogin>("UserLogin");
  pick_and_delivery::UserLogin srvUserLogin;
  
  
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
  
  ROS_INFO("SERVER RESPONSE: %s",srvUserLogin.response.login.c_str());
  //continuo
  while(true)
  {
	  
  }  
	  
  return 0;
}
