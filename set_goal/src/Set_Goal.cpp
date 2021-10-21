#include "ros/ros.h"
#include <vector>
#include "std_msgs/String.h"
#include "set_goal/NewGoal.h"
#include "geometry_msgs/PoseStamped.h"
#include "tf/tf.h"
#include "tf2_msgs/TFMessage.h"
#include <sstream>
#include <ros/ros.h>
#include <tf2_ros/transform_listener.h>
#include <geometry_msgs/TransformStamped.h>
#include <nav_msgs/Odometry.h>
#include <sensor_msgs/LaserScan.h>
#include <tf2_msgs/TFMessage.h>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2_ros/transform_broadcaster.h>
#include "pick_and_delivery/InfoComunication.h"

std::vector<float> target_position(2.0); //nuova posizione mentre mi sposto(ad ogni passo)
std::vector<float> old_position(2.0);    //vecchia posizione mentre mi sposto(ad ogni passo)
std::vector<float> current_position(2.0);

geometry_msgs::PoseStamped new_goal_msg;
tf2_ros::Buffer tfBuffer; //buffer per le trasformate

ros::Publisher pubStatusInfo;

size_t n=10;
int message_published=0;

//se ci stiamo spostando setto a 1 altrimenti 0
int cruising=0;
time_t T=10;
//prende il messaggio new_goal ovvero il messaggio con all' interno x,y e theta

//il topic(rostopic list) è /move_base_simple/goal  (rostopic info). Il messaggio
//da mandare per settare un nuovo goal è del tipo geometry_msgs/PoseStamped
//per settare un messaggio da terminale rostopic echo /move_base_simple/goal


void SetGoal_CallBack(const set_goal::NewGoal& new_goal)
{
	/* i campi del messaggio sono:
	 * -seq; numero di sequenza
	 * -stamp; quando mandare il messaggio(ora)
	 * -id;identifica
	 * -x,y,z posizion e orientation; per la posizione ce si vuole
	 * */
	new_goal_msg.header.seq=n;
	n++;
	new_goal_msg.header.stamp=ros::Time::now();
	new_goal_msg.header.frame_id="map";
	
	new_goal_msg.pose.position.x=new_goal.x;
	new_goal_msg.pose.position.y=new_goal.y;
	new_goal_msg.pose.position.z=0;
	
	
	new_goal_msg.pose.orientation.x=0;
	new_goal_msg.pose.orientation.y=0;
	new_goal_msg.pose.orientation.z=0;
	new_goal_msg.pose.orientation.w=new_goal.theta;
	
	//per fare in modo che la callback viene chiamata solo una volta
	message_published=1;
	
	//mi sto spostando
	cruising=1;
	
	
	//salvare la goal position
	target_position[0]=new_goal_msg.pose.position.x;
	target_position[1]=new_goal_msg.pose.position.y;
	
	
	
	//pub.publosh(new_goal_msgs);


}


//callback che ci dice quale è la posizione del robot in quell istante
void position_CallBack(const tf2_msgs::TFMessage& tf)
{
	int transform_ok=tfBuffer.canTransform("map","base_link",ros::Time(0));
	if(transform_ok!=0)//posso transformare 
	{
		geometry_msgs::TransformStamped transformStamped;
		transformStamped = tfBuffer.lookupTransform("map","base_link",ros::Time(0));
		
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
		pick_and_delivery::InfoComunication infoSTATUS; //status 0 ancora in movimento,-1 ERRORE,1 OK
		infoSTATUS.status=0;
		infoSTATUS.info="In movimento...";
		//prendo la distanza tra la posizione corrente e quella precendete.
		//se minore di un certo valore significa che mi sono bloccato
		float distance;
		distance=sqrt(pow(current_position[0]-old_position[0],2)+pow(current_position[1]-old_position[1],2));
		if(distance < 0.8)
		{
			infoSTATUS.status=-1;
			infoSTATUS.info="BLOCCATO";
			ROS_INFO("PROBLEMAAA!!!");
		}
		
		//per verificare se sono arrivato al goal
		//controllo tra la posizione corrente e la posizione target
		//se sono arrivato termino di muovermi(cruising=0)
		if(sqrt(pow(current_position[0]-target_position[0],2)+pow(current_position[1]-target_position[1],2)) < 1.5)
		{
			ROS_INFO("Arrivato al goal");
			infoSTATUS.status=1;
			infoSTATUS.info="Arrivato al goal";
			cruising=0; //non sono più in movimento
		}
		pubStatusInfo.publish(infoSTATUS);
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
		pick_and_delivery::InfoComunication infoSTATUS; //status 0 ancora in movimento,-1 ERRORE,1 OK
		infoSTATUS.status=0;
		infoSTATUS.info="In movimento...";
		float distance;
		distance=sqrt(pow(current_position[0]-target_position[0],2)+pow(current_position[1]-target_position[1],2));
		if(distance> 0.5)
		{
			infoSTATUS.status=-1;
			infoSTATUS.info="TIMEOUT: Troppo tempo";
			ROS_INFO("TIMEOUT: Goal non raggiunto in tempo");
		}
		pubStatusInfo.publish(infoSTATUS);

	}
}

int main(int argc,char **argv)
{
	ros::init(argc,argv,"Set_Goal");
	
	ros::NodeHandle n;
	ros::Publisher pub=n.advertise<geometry_msgs::PoseStamped>("/move_base_simple/goal",1000);
	pubStatusInfo=n.advertise<pick_and_delivery::InfoComunication>("infoComunication",1000);

	
    tf2_ros::TransformListener tfListener(tfBuffer);
	
	ros::Rate loop_rate(T);
	
	//callback usata per ricevere i messaggi di goal del robot.
	//Quando viene pubblicato un messagio di New_Goal viene chiamata la callback che preleva 
	//il messaggio e vede quale è la posizione di goal da raggiungere
	//OSS: potevo passare anche i paramentri del goal al programma quando lo avvio(rosrun)
	ros::Subscriber sub=n.subscribe("New_Goal",1000,SetGoal_CallBack);
	
	
	//callback usata per vedere la posizione del robot ad ogni istante
	//mi iscrivo al topic tf e chiamare la position_callback.
	//Ogni volta che vengono pubblicati messaggi sul topic tf si avvia la position_callback
    //in modo da avere sempre l' ultima posione del roboto aggiornata
	ros::Subscriber sub_tf=n.subscribe("tf",1000,position_CallBack);
	
	
	//settare callback con un certo periodo di tempo
	ros::Timer timer1=n.createTimer(ros::Duration(0.5),check1_CallBack);
	ros::Timer timer2=n.createTimer(ros::Duration(200),check2_CallBack);

	int count=0;
	while(ros::ok())
	{
		if(message_published!=0)
		{
			ROS_INFO("Publishing a new goal position");
			pub.publish(new_goal_msg);
			message_published=0;
		}
		
		ros::spinOnce();
		loop_rate.sleep();
		
		++count;
		
		
	}
	return 0;

}

/*ESECUZIONE*/
/*AVVIARE I VARI NODI(MAPPA,RVIZ, ECC) E FARE I SOLITI PASSAGGI
 * AVVIARE IL NODO PER SETTARE IL GOAL
 * rosrun set_goal Set_Goal
 * 
 * PUBBLICARE UN MESSAGGIO POSIZIONE SUL TOPIC PER INDICARE IL GOAL DA RAGGIUNGERE
 * rostopic pub /New_Goal set_goal/NewGoal "x: 53.68 y: 18.0 theta: 0.02 
 * 
 * 
 * OSS: se vaccio view frames mi da un pdf di tutte le trasformate 
 * 
 * 
 * */
