#include <bits/stdc++.h>
#include <arpa/inet.h> //for networking
#include <sys/socket.h> // for socket communication
#include <unistd.h> //for unix operating system

#define ip_size 36
#define bufsz 1024
#define inf 1000000
#define MAXRUT 100

using namespace std;

struct sockaddr_in own_address;
int sockfd;
int bind_flag;

socklen_t addrlen;

char my_ip[ip_size];
char server_ip[ip_size]="192.168.10.100";
struct sockaddr_in my_address;
struct sockaddr_in other_address;

struct Router{
	char ip_addr[ip_size];
	char nexthop_addr[ip_size];
	int cost;
	struct sockaddr_in address;
	int mssgco;
	int up;
	
	Router(char *ip,int c){
		strcpy(ip_addr,ip);
		cost=c;	
		up=1;
		mssgco=0;
		address.sin_family = AF_INET;
		address.sin_port = htons(4747);
		address.sin_addr.s_addr = inet_addr(ip_addr);
		
		

	}
	bool match(char *ano_ip){
		return !strcmp(ip_addr,ano_ip);
	}

};

void setup_own(Router cur){
	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	bind_flag = bind(sockfd, (struct sockaddr*) &cur.address, sizeof(sockaddr_in));
	if(bind_flag<0){
		printf("Connection failed.\n");
		return;
	}
	printf("Connection stablished succesfully.\n");

}


vector<Router>routers,neigh;

int  inrouter(char *check_ip){
	for(int i=0;i<routers.size();i++){
		if(routers[i].match(check_ip)){
			return i;
		}
	}
	return -1;

}

void show_routing_table(){
	printf("Destination    Next Hop       Cost\n");
	printf("-----------   -----------  ...........\n");
	for(int i=0;i<routers.size();i++){
		printf("%s  %s 	%d\n",routers[i].ip_addr,routers[i].nexthop_addr,routers[i].cost);
	
	}
	printf("\n");
}

void send_routing_table();
int get_id(char *x);
int get_id_router(char *x);
int get_my_cost(char *x);
void updown_link(int now);

int main(int argc, char *argv[]){

	
	char buffer[bufsz];
	int bytes_received;
	FILE *topo;
	char delim[]=" ";
	int totalmssg=0;
	

	if(argc < 3){
		printf("Wrong input format.\n Right format : %s <ip address> topo.txt\n", argv[0]);
		exit(1);
	}
	strcpy(my_ip,argv[1]);
	Router Cur(argv[1],0);
	Router Server(server_ip,inf);
	setup_own(Cur);
	topo=fopen(argv[2],"r");
	if(topo==NULL){
		printf("File can not open : %s\n",argv[2]);
		exit(1);
	}
	
	while(fgets(buffer,bufsz,topo)!=NULL){
		char *ptr=strtok(buffer,delim);
		char *src;
		char *dest;
		int cost;
		int i=0;
		while(ptr!=NULL){
			if(i==0) src=ptr;
			else if(i==1) dest=ptr;
			else cost=atoi(ptr);
			i++;
			ptr=strtok(NULL,delim);
		}
		if(strcmp(argv[1],src)==0){
			int x=inrouter(dest);
			if(x==-1){
				Router new_router(dest,cost);
				strcpy(new_router.nexthop_addr,dest);
				new_router.up=1;
				routers.push_back(new_router);
			}else{
				routers[x].cost=cost;
				strcpy(routers[x].nexthop_addr,dest);
				routers[x].up=1;
			}
			
		}else if(strcmp(argv[1],dest)==0){
			int x=inrouter(src);
			if(x==-1){
				Router new_router(src,cost);
				strcpy(new_router.nexthop_addr,src);
				new_router.up=1;
				routers.push_back(new_router);
			}else{
				routers[x].cost=cost;
				strcpy(routers[x].nexthop_addr,src);
				routers[x].up=1;
			}

		}else{
			int x=inrouter(src);
			if(x==-1){
				Router new_router(src,inf);
				strcpy(new_router.nexthop_addr, "------------");
				new_router.up=1;
				routers.push_back(new_router);
			}
			x=inrouter(dest);
			if(x==-1){
				Router new_router(dest,inf);
				strcpy(new_router.nexthop_addr, "------------");
				new_router.up=1;
				routers.push_back(new_router);
			}

		}

	}
	show_routing_table();
	fclose(topo);
	for(int i=0;i<routers.size();i++){
		if(!strcmp(routers[i].ip_addr,routers[i].nexthop_addr)){
			neigh.push_back(routers[i]);
		}
	}
	strcpy(buffer,"");
	
	

	while(true){
		
		bytes_received = recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr*) &other_address, &addrlen);
		if(bytes_received<0){
			continue;
		}
		//buffer[bytes_received]='\0';
		//printf("[%s:%d]: %s\n", inet_ntoa(other_address.sin_addr), ntohs(other_address.sin_port), buffer);
		char current_ip[ip_size]="";
		strcpy(current_ip,inet_ntoa(other_address.sin_addr));
		if(!strcmp(current_ip,server_ip)){
		if(!strncmp("show",buffer,4)){
		//	printf("%s\n",buffer);
			int a,b,c,d;
			a=(buffer[4]+256)%256;
			b=(buffer[5]+256)%256;
			c=(buffer[6]+256)%256;
			d=(buffer[7]+256)%256;
			char show_ip[ip_size];
			sprintf(show_ip,"%d.%d.%d.%d",a,b,c,d);
			
			if(!strcmp(Cur.ip_addr,show_ip)){
				show_routing_table();
			}
		}else if(!strncmp("clk",buffer,3)){
			 send_routing_table();
			 updown_link(totalmssg);
			 totalmssg++;
			// receive_routing_table();
			//printf("here");
		}
		else if(!strncmp("send",buffer,4)){
			int ip1[4];
			 for(int i=4;i<=7;i++){
				ip1[i-4]=(buffer[i]+256)%256;
			 }
			 
			 char ipone[ip_size];
			 sprintf(ipone,"%d.%d.%d.%d",ip1[0],ip1[1],ip1[2],ip1[3]);
			 int ip2[4];
			 for(int i=8;i<=11;i++){
				ip2[i-8]=(buffer[i]+256)%256;
			 }
			 
			 char iptwo[ip_size];
			 sprintf(iptwo,"%d.%d.%d.%d",ip2[0],ip2[1],ip2[2],ip2[3]);
			 int cost[4];
			 int k=0;
			 for(int i=12;i<=13;i++){
				cost[i-12]=(int)buffer[i];
				k++;
			 }
			 int now_cost=0;
			 for(int i=0;i<k;i++){
				 if(i==0){
					 if(cost[i]<0){
						 cost[i]+=256;
					 }
					 now_cost+=cost[i];
				 }
				 else{
					  if(cost[i]<0){
						 cost[i]+=256;
					 }
					  now_cost+=(256*cost[i]);

				 }
			 }
			 char mssg[now_cost+1];
			 for(int i=14;i<14+now_cost;i++){
				 mssg[i-14]=buffer[i];
				 if(i==(14+now_cost-1)){
					 mssg[i-14+1]='\0';
				 }
			 }

			 if(strcmp(my_ip,ipone)!=0){
				 continue;
			 }
			 int id=get_id_router(iptwo);
			
			id=get_id_router(routers[id].nexthop_addr);
			
			printf("%s packet forwarded to %s  (printed by %s)\n",mssg,routers[id].ip_addr,my_ip);
			char new_mssg[bufsz];
			sprintf(new_mssg,"frwd %s %d %s",iptwo,now_cost,mssg);
			//printf("%s\n",new_mssg);
			sendto(sockfd, new_mssg, bufsz, 0, (struct sockaddr*) &routers[id].address, sizeof(sockaddr_in));
		}
		else if(!strncmp("cost",buffer,4)){
			// for(int i=0;i<strlen(buffer);i++){
			// 	printf("%d ",buffer[i]);
			// }
			//printf("\n");
			
			 int ip1[4];
			 for(int i=4;i<=7;i++){
				ip1[i-4]=(buffer[i]+256)%256;
			 }
			 
			 char ipone[ip_size];
			 sprintf(ipone,"%d.%d.%d.%d",ip1[0],ip1[1],ip1[2],ip1[3]);
			 int ip2[4];
			 for(int i=8;i<=11;i++){
				ip2[i-8]=(buffer[i]+256)%256;
			 }
			 
			 char iptwo[ip_size];
			 sprintf(iptwo,"%d.%d.%d.%d",ip2[0],ip2[1],ip2[2],ip2[3]);
			 int cost[4];
			 int k=0;
			 for(int i=12;i<=13;i++){
				cost[i-12]=(int)buffer[i];
				k++;
			 }
			 
			 //printf("%s %s \n",ipone,iptwo);
			int now_cost=0;
			 for(int i=0;i<k;i++){
				 if(i==0){
					 if(cost[i]<0){
						 cost[i]+=256;
					 }
					 now_cost+=cost[i];
				 }
				 else{
					  if(cost[i]<0){
						 cost[i]+=256;
					 }
					  now_cost+=(256*cost[i]);

				 }
			 }
			if(!strcmp(ipone,my_ip)){
				int id=get_id(iptwo);
				if(id==-1){
					continue;
				}
				neigh[id].cost=now_cost;
				//printf("%s %d\n",neigh[id].ip_addr,neigh[id].cost);
				int ano_id=get_id_router(iptwo);
				if(ano_id!=-1){
					
						routers[ano_id].cost=neigh[id].cost;
						strcpy(routers[ano_id].nexthop_addr,iptwo);
					
				}

			//	printf("%s\n",neigh[id].ip_addr);
			}
			else if(!strcmp(iptwo,my_ip)){
				int id=get_id(ipone);
				if(id==-1){
					continue;
				}
				

				neigh[id].cost=now_cost;
				int ano_id=get_id_router(ipone);//printf("%s %d\n",neigh[id].ip_addr,neigh[id].cost);
				if(ano_id!=-1){
						routers[ano_id].cost=neigh[id].cost;
						strcpy(routers[ano_id].nexthop_addr,ipone);
					
				}
			}

			 
			// printf("%d %d\n",cost[0],cost[1]);
			// int value=atoi(cost);
			// printf("%d\n",value);

			 
			 
		}
		}
		else{
			if(!strcmp(my_ip,current_ip)){
				continue;
			}
			if(!strncmp("RTable from ",buffer,12)){
			char join[bufsz];
			char *ptr=strtok(buffer,"\n");
			 int i=0;
			 int my_cost=0;

			 while(ptr!=NULL){
				
				//printf("%s\n",ptr);
			 	if(i==0 and strncmp(ptr,"RTable from ",12)!=0){
			 		break;
			 	} 
			 	if(i==0 and !strncmp(ptr,"RTable from ",12)){
			 		sscanf(ptr,"RTable from %s\n",join);
			 		if(strcmp(my_ip,join)==0){
						 break;
					 }
					 my_cost=get_my_cost(join);
					 if(get_id(join)!=-1)
					 neigh[get_id(join)].mssgco=totalmssg;

					// printf("%s %d\n",join,neigh[get_id(join)].mssgco);
			 	}
			 	if(i>0){
					 
					
					char src[ip_size];
					char dest[ip_size];
					int cost;
					char p[bufsz];
					int k=0;
					int j=0;
					//printf("%s\n",ptr);
					for(int i=0;i<strlen(ptr);i++){
						if(ptr[i]==' '){
							p[k]='\0';
							k=0;
							if(j==0){
								strcpy(src,p);

							}
							else if(j==1){
								strcpy(dest,p);
							}
							
							j++;

						}
						else{
							p[k]=ptr[i];k++;
							if(i==strlen(ptr)-1){
								p[k]='\0';
								cost=atoi(p);
							}
							
						}
					}
					if(!strcmp(src,my_ip)){
						ptr=strtok(NULL,"\n");
						continue;
					}
					//printf("%s %s %d\n",src,dest,cost);
					int id=get_id_router(src);
					if(id==-1){
						ptr=strtok(NULL,"\n");
						continue;
					}
					
					int now_cost=routers[id].cost;
					//printf("%s %d\n",routers[id].ip_addr,now_cost);
					if(cost+my_cost<now_cost){
						//printf("%s %s %d\n",src,dest,cost);
						routers[id].cost=cost+my_cost;
						strcpy(routers[id].nexthop_addr,join);
						//printf("%s\n",join);
					}
					else if(!strcmp(routers[id].nexthop_addr,join)){
						routers[id].cost=cost+my_cost;
					}
					 
			 	}
			 	i++;
				// printf("%d\n",i);
				
			 	ptr=strtok(NULL,"\n");
			 }
			 for(int i=0;i<neigh.size();i++){
				 int id=get_id_router(neigh[i].ip_addr);
				 if(!strcmp(routers[id].ip_addr,routers[id].nexthop_addr)){
					 continue;
				 }
				 char realhop[ip_size];
				 strcpy(realhop,routers[id].nexthop_addr);
				 for(int j=0;j<routers.size();j++){
					 if(!strcmp(routers[j].nexthop_addr,neigh[i].ip_addr)){
						 strcpy(routers[j].nexthop_addr,realhop);
					 }
				 }
			 }

			}else if(!strncmp("frwd",buffer,4)){
				char next_ip[ip_size];
				int cost;
				char mssg[bufsz];
				sscanf(buffer,"frwd %s %d %s",next_ip,&cost,mssg);
				//printf("%s %d %s\n",next_ip,cost,mssg);
				if(!strcmp(next_ip,my_ip)){
					printf("%s packet reached destination (printed by %s)\n",mssg,my_ip);
					continue;
				}
				int id=get_id_router(next_ip);
				id=get_id_router(routers[id].nexthop_addr);
				char new_mssg[bufsz];
				sprintf(new_mssg,"frwd %s %d %s",next_ip,cost,mssg);
				printf("%s packet forwarded to %s (printed by %s)\n",mssg,routers[id].ip_addr,my_ip);
				sendto(sockfd, new_mssg, bufsz, 0, (struct sockaddr*) &routers[id].address, sizeof(sockaddr_in));


			}
		}
		
		//sendto(sockfd, buffer, 1024, 0, (struct sockaddr*) &server_address, sizeof(sockaddr_in));
	}
	

	close(sockfd);

	return 0;

}

int get_my_cost(char *x){
	for(int i=0;i<routers.size();i++){
		if(routers[i].match(x)){
			return routers[i].cost;
		}
	}
	return inf;
}

int get_id(char *x){
	
	
	for(int i=0;i<neigh.size();i++){
//printf("%s %s\n",x,routers[i].ip_addr);
		if(neigh[i].match(x)){
			
			return i;
		}
	}
	return -1;
}

int get_id_router(char *x){
	
	
	for(int i=0;i<routers.size();i++){
//printf("%s %s\n",x,routers[i].ip_addr);
		if(routers[i].match(x)){
			
			return i;
		}
	}
	return -1;
}
void updown_link(int now){
	

	for(int i=0;i<neigh.size();i++){
		//printf("%s %d\n",neigh[i].ip_addr,neigh[i].cost);
		if(neigh[i].up==0){
			if(neigh[i].mssgco>0){
				neigh[i].up=1;
				neigh[i].mssgco=now;
				
				for(int j=0;j<routers.size();j++){
				if(!strcmp(routers[j].ip_addr,neigh[i].ip_addr)){
					routers[j].cost=neigh[i].cost;
					strcpy(routers[j].nexthop_addr,neigh[i].ip_addr);
				}
				}
				//routers[i].cost=neigh[i].cost;
			}
		}
		if(neigh[i].mssgco<(now-3)){
			neigh[i].up=0;
			neigh[i].mssgco=0;
			for(int j=0;j<routers.size();j++){
				if(!strcmp(routers[j].nexthop_addr,neigh[i].ip_addr)){
					routers[j].cost=inf;
					strcpy(routers[j].nexthop_addr,"------------");
				}
			}
			

		}
		//routers[i].mssgco=0;
	}
	for(int i=0;i<routers.size();i++){
	//	printf("%s %d %d\n",routers[i].ip_addr,routers[i].mssgco,routers[i].up);
	}
	

}
void send_routing_table(){
	char mssg[bufsz];
	char join[bufsz]="";
	strcat(join,"RTable from ");
	strcat(join,my_ip);
	strcat(join,"\n");
	strcat(mssg,join);
	for(int i=0;i<routers.size();i++){

	 	sprintf(join,"%s %s %d\n",routers[i].ip_addr,routers[i].nexthop_addr,routers[i].cost);
	 	strcat(mssg,join);
	}
	//printf("%s\n",mssg);
	for(int i=0;i<neigh.size();i++){
			if(neigh[i].up)
			sendto(sockfd, mssg, bufsz, 0, (struct sockaddr*) &neigh[i].address, sizeof(sockaddr_in));
		
	}
	strcpy(mssg,"");

}
