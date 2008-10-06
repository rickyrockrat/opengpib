/** \file ******************************************************************
\n\b File:        gpib.c
\n\b Author:      Doug Springer
\n\b Company:     DNK Designs Inc.
\n\b Date:        10/06/2008  1:44 am
\n\b Description: common gpib functions...to talk to prologix.
*/ /************************************************************************
Change Log: \n
$Log: not supported by cvs2svn $
*/
#include "gpib.h"
extern char buf[BUF_SIZE];
/***************************************************************************/
/** .
\n\b Arguments:
len is max_len for msg.
\n\b Returns: number of chars read.
****************************************************************************/
int read_string(struct serial_port *p, char *msg, int len)
{
	int i,count;

	fd_set rd;
	struct timeval tm;
	tm.tv_sec=1;
	tm.tv_usec=0;
	FD_ZERO(&rd);
	FD_SET(p->handle, &rd);
	select (p->handle+1, &rd, NULL, NULL, &tm);
#if 0		
	/*printf("RD\n"); */
	
	for (i=0; i<len;++i){
		int x;

		for (count=0;count<20;++count){
			/*printf("Sl\n"); */
			if(select (p->handle+1, &rd, NULL, NULL, &tm) < 0)	{
				if ((errno != EINTR) && (errno != EAGAIN)
				    && (errno != EINVAL)) {
					printf("Key input err\n");
					return (-1);
				} 
			}	
			if(0 != FD_ISSET(p->handle, &rd)){
				break;
			}
		}
		if(1 != (x=read(p->handle,&msg[i],1)) )
			perror("");
			/*usleep(100); */
		
		if(x != 1 ){
			
			printf("Timeout waiting on char\n");
			return -1;
		}
		printf("%c",msg[i]);
		if(msg[i]=='\n' || msg[i] == '\r')/* || msg[i] == '\r') */
			break;
	}
#else
		for (i=0; i<len;++i){
		int x;
		for (count=0;count<2000;++count){
			if(0< (x=read(p->handle,&msg[i],len-i)) ){
				i+=x-1;
				break;
			}
				
			usleep(100);
		}
		if(0 == x){	
			if(0 == i || msg[i-1] != '\r' )	{
				printf("Timeout waiting on char\n");
				return -1;
				
			}else{
				--i;
				break;	
			}
		}	
		/*printf("%c",msg[i]); */
		if(msg[i]=='\n' )/* || msg[i] == '\r') */
			break;
	 }
#endif
		
#if 0	
	if( (rtn=read(p->handle,msg,len)) <1 ) {
    if( errno == EAGAIN || rtn == 0) /* EAGAIN - no data avail */
      usleep(60); /**for 15200 baud  */
    else  {
      printf("We had a Read Error on %s.\n",p->phys_name);
			return -1;
    }
  }
#endif
	while(i && (buf[i]=='\r' || buf[i]=='\n'))
		--i;
	++i;
	buf[i]=0;
	return i;
}
/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int write_string(struct serial_port *p, char *msg, int len)
{
	int rtn,i;
	if(NULL ==msg){
		printf("msg null\n");
		return -1;
	}
	if(0 == len)
		i=strlen(msg);
	else
		i=len;
	if((rtn=write(p->handle,msg,i)) <0) {/* error writing */
    printf("Error Writing to %s\n",p->phys_name);
		return -1;
  }
	/*printf("wrote %d bytes\n%s\n",rtn,msg); */
	if(rtn != i)
		printf("Write mis-match %d != %d\n",rtn,i);
	usleep(500); 
	return rtn;	
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
void verify(struct serial_port *p, char *msg)
{
	int i;
	i=sprintf(buf,"%s\r",msg);
	write_string(p,buf,i);
	read_string(p,buf,BUF_SIZE);
	/*printf("%s = %s\n",msg,buf);	 */
}

/***************************************************************************/
/** .
\n\b Arguments:
\n\b Returns:
****************************************************************************/
int init_prologix(struct serial_port *p, int inst_addr)
{
	int i;
	printf("Init Prologix controller\n");
	read_string(p,buf,BUF_SIZE);
	/*make sure auto reply is on*/
	write_string(p,"++auto 1\r",0);
	write_string(p,"++ver\r",0);
	/*write_string(p,"++read\r",0); */
	
	if(-1 == (i=read_string(p,buf,BUF_SIZE)) ){
		printf("%s:Unable to read from port on ver\n",__func__);
		return -1;
	}
	/*printf("Got %d bytes\n",i); */
	if(NULL == strstr(buf,"Prologix")){
		printf("%s:Unable to find correct ver in\n%s\n",__func__,buf);
		return -1;
	}
	printf("Talking to Controller '%s'\n",buf);
	/*Then set to Controller mode */
	write_string(p,"++mode 1\r",0);
	
	/*Set the address to talk to (2, usually) */
	sprintf(buf,"++addr %d\r",inst_addr);
	write_string(p,buf,0);
	/*Set the eoi mode (EOI after cmd) */
	write_string(p,"++eoi 1\r",0);
	/*Set the eos mode (LF) */
	write_string(p,"++eos 2\r",0);
	verify(p,"++addr");
	verify(p,"++eoi");
	verify(p,"++eos");
	verify(p,"++auto");
	return 0;
}
