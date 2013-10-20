/*MADE BY:
       Anubhav Gupta 
       Arpit Gupta
       Anurag Atri*/

#include <stdio.h>
#include <conio.h>
#include <time.h>
#include <string.h>

/* Disk is of 32 MB */

#define SIZE_OF_SECTOR 1024 
#define NO_OF_SECTORS 32768
#define SIZE_OF_RECYCLE_BIN 1048576  /* 1 MB */
#define SIZE_OF_DISK 33554432       /* 32MB */
#define FEPS 256  /* Fat Entries Per Sector */
#define DEPS 32   /* Directory Entries Per Sector */
#define SIZE_OF_PASSWORD 6
#define MAX_PARENTS 50
#define NAME_SIZE 11

/* MASTER BOOT RECORD */
struct MBR /* size = 256integers */
{
       int start_of_fat;
       int start_of_root;
       int start_of_first_fb;
       int start_of_recycle_bin;
       int padding[252];
};

struct dir_entry
{
       char name[NAME_SIZE];        /*8 max for name, 3 for extension*/
       
       char dir_attributes;  
       /* format: XXPVDHAR: Password:Valid:Directory:Hidden:Archive:ReadOnly*/
       
       int starting_address;
       int size;             /*actual size*/
       int size_on_disk;     /*size in terms of sectors*/
       time_t date_created;
       time_t date_modified;
};

struct dir_struct                   /*represents a directory structure*/
{
       struct dir_entry dir_array[DEPS];
};

/* Buffer to store data temporarily while reading or writing a sector */
union buffer
{
   struct MBR mbr_buffer;
   int FAT_buffer[SIZE_OF_SECTOR/sizeof(int)];
   struct dir_struct dirStruct_buffer;
   char data_buffer[SIZE_OF_SECTOR];
}buf;

enum { mbr, FAT, dirStruct, data };
/* mbr = 0
   FAT = 1
   dirStruct = 2
   data = 3
*/
 
/****************************************************************************/
/*reading a sector*/

void readSector(FILE *fp, int choice, int offset)
{
    /*
       case 0: read mbr
       case 1: read a FAT sector
       case 2: read a directory sector
       case 3: read a data sector
    */
    switch(choice)
    {
        case 0: rewind(fp);
                fread(&buf.mbr_buffer,sizeof(buf.mbr_buffer),1,fp);
                break;
        case 1: rewind(fp);
                fseek(fp,SIZE_OF_SECTOR*(1+offset),SEEK_SET);
                fread(&buf.FAT_buffer,sizeof(buf.FAT_buffer),1,fp);
                break;
        case 2: rewind(fp);
                fseek(fp,sizeof(buf.dirStruct_buffer)*offset,SEEK_SET);
                fread(&buf.dirStruct_buffer,sizeof(buf.dirStruct_buffer),1,fp);
                break;
        case 3: rewind(fp);
                fseek(fp,SIZE_OF_SECTOR*offset,SEEK_SET);
                fread(buf.data_buffer,sizeof(buf.data_buffer),1,fp);
                break;
    }
}

/****************************************************************************/
/* writing a sector */

void writeSector(FILE *fp, int choice, int offset)
{
    /*
       case 0: write mbr
       case 1: write a FAT sector
       case 2: write a directory sector
       case 3: write a data sector
    */
    switch(choice)
    {
        case 0: rewind(fp);
                fwrite(&buf.mbr_buffer,sizeof(buf.mbr_buffer),1,fp);
                break;
        case 1: rewind(fp);
                fseek(fp,SIZE_OF_SECTOR*(1+offset),SEEK_SET);
                fwrite(&buf.FAT_buffer,sizeof(buf.FAT_buffer),1,fp);
                break;
        case 2: rewind(fp);
                fseek(fp,sizeof(buf.dirStruct_buffer)*offset,SEEK_SET);
                fwrite(&buf.dirStruct_buffer,sizeof(buf.dirStruct_buffer),1,fp);
                break;
        case 3: rewind(fp);
                fseek(fp,SIZE_OF_SECTOR*offset,SEEK_SET);
                fwrite(buf.data_buffer,sizeof(buf.data_buffer),1,fp);
                break;
    }
}



/****************************************************************************/
/*traversal*/

int family_stack[MAX_PARENTS];
int tos;

void push(int curdir)
{
     int i;
     if(tos == (MAX_PARENTS - 1))
          printf("DISK IS FULL");
     else
          family_stack[++tos] = curdir;
}

int pop()
{
    if(tos==-1)
          printf("NO member left..\n");
    else
          return family_stack[--tos];
}
/***************************************************************************/
/*prints path from root to current directory
  in the form root\...\current directory>*/
void traversal(FILE *fp)
{
     int i,j,flag,parent;
     printf("\nroot\\");
     //for every entry in family stack, starting from root    
     for(i=0; i<tos; i++)
     {
       flag = 0;
       parent = family_stack[i];
       readSector(fp,dirStruct,parent);    //read in its directory structure
       while (1)
       {
             /*in this directory struc, look for child
                        corresponding to next entry in family stack*/
            for (j=0;j<DEPS;j++)           
            {
                if((buf.dirStruct_buffer.dir_array[j].starting_address) 
                                                         == family_stack[i+1] )
                {
                    printf("%s\\",buf.dirStruct_buffer.dir_array[j].name);
                    flag = 1;            //once found print its name and 
                    break;              //repeat for next entry of family_stack
                }
            }
            if(flag==1)
                break;
            
            /*if no such child is found, then look in next directory structure
                 corresponding to the directory*/
                 
            /* FAT[temp] != -1, since family stack can never contain 
               erroneous entries because of the way it is constructed*/
            
            j = parent/FEPS;
            
            //read in the next directory structure, and look again
            readSector(fp,FAT,j);
            
            parent = buf.FAT_buffer[parent%FEPS];
          
            readSector(fp,dirStruct,parent);
        }
     }
     
     printf("\b>");//remove extra slash at the end, and print an arrow
}
/*****************************************************************************/
/*initializing disk*/

int initialize_disk(FILE *fp,int curdir)
{
     int i,k;
     long j;                    
     int temp,temp1,temp2;
 
     tos = -1;     
     readSector(fp,mbr,0);
     
     buf.mbr_buffer.start_of_fat = 1;
     //MBR: 1 block
     
     buf.mbr_buffer.start_of_root = 129;            
     //FAT:128 blocks
     
     temp1 = buf.mbr_buffer.start_of_root;          
     //to be used for initializing root structure
     
     buf.mbr_buffer.start_of_recycle_bin = 130;
     
     temp = buf.mbr_buffer.start_of_recycle_bin;
     
     buf.mbr_buffer.start_of_first_fb = 131;        
     //first is root dir structure
     
     temp2 = buf.mbr_buffer.start_of_first_fb;      
     //to be used for constructing list of free sectors
     
     writeSector(fp,mbr,0);   
     //write mbr into disk
     
     
     /*FAT[i] = -2 implies ith sector stores system information
       FAT[i] = -1 implies ith sector is end of file/directory
       FAT[i] = j implies either i and j are consecutive free sectors
                  or consecutive data sectors*/
     /*----------------------------------------------------------------*/ 
     /* first sector */                                     
     readSector(fp,FAT,0);
     for(i=0; i<129; i++)
         buf.FAT_buffer[i] = -2;      /*system info*/
              
     buf.FAT_buffer[temp1] = -1;      /*end of root dir struct*/
     buf.FAT_buffer[temp] = -1;       /*end of recycle bin dir struct*/
     
     for(i=temp2; i<FEPS; i++)       
        buf.FAT_buffer[i] = i+1;      /*constructing list of free sectors*/
     writeSector(fp,FAT,0);
     k = i;
     /*---------------------------------------------------------------------*/
     /*all are free sectors*/
     for(j=2; j<(NO_OF_SECTORS-1); j++)
     {
           readSector(fp,FAT,j-1);
           for(i=0; i<FEPS; i++)
              buf.FAT_buffer[i] = k++;
           writeSector(fp,FAT,j-1);
     }
     /*--------------------------------------------------------------------*/  
     /* last sector */
     j = NO_OF_SECTORS-1;
     
     readSector(fp,FAT,j-1);
     for(i=0; i<FEPS; i++)
        buf.FAT_buffer[i] = k++;
     buf.FAT_buffer[--i] = -1;      
     writeSector(fp,FAT,j-1);                          
     /*---------------------------------------------------------------------*/         
     /* invalidating the root */
     readSector(fp,dirStruct,temp1);
     for (i=0;i<DEPS;i++)
         buf.dirStruct_buffer.dir_array[i].dir_attributes = 0;
       
     writeSector(fp,dirStruct,temp1);  
     curdir = temp1;   /* setting current directory initially to root */
     
     /* invalidating recycle bin */
     readSector(fp,dirStruct,temp);
     for (i=0;i<DEPS;i++)
         buf.dirStruct_buffer.dir_array[i].dir_attributes = 0;
     for (i=1;i<DEPS;i=i+2)
         buf.dirStruct_buffer.dir_array[i].starting_address = -1;
     writeSector(fp,dirStruct,temp);
         
     printf("\nThe DISK has been initialised... \n");
     
     push(curdir);
     return curdir;
}

/***************************************************************************/











/****************************************************************************/


/* dir is first sector of a particular directory */
void remove_struct(int startdir, int dir,FILE *fp)
{
   int tempfb,temp1,temp2;
   int i;

   /* first sector */
   if(startdir == dir)
     return;
   
   readSector(fp,dirStruct,dir);
   for(i=0;i<32;i++)
     {
        /* if dere is any valid entry */
        if((buf.dirStruct_buffer.dir_array[i].dir_attributes&16)==16)
              return;
     }
               
   /*if all entries are invalid, den free this structure*/
    readSector(fp,FAT,startdir/256);
    temp1 = startdir;
    
    while(buf.FAT_buffer[temp1%256]!=dir)
      {
         temp1 = buf.FAT_buffer[temp1%256];
         readSector(fp,FAT,temp1/256);
      }


    //updating mbr.....
    readSector(fp,mbr,0);
    tempfb = buf.mbr_buffer.start_of_first_fb;
    buf.mbr_buffer.start_of_first_fb = dir;
    writeSector(fp,mbr,0);

    readSector(fp,FAT,dir/256);
    temp2 = buf.FAT_buffer[dir%256];
       
    //updating free block list.....
    buf.FAT_buffer[dir%256] = tempfb;
    writeSector(fp,FAT,dir/256);
       
    //updating FAT.......
    readSector(fp,FAT,temp1/256);
    buf.FAT_buffer[temp1%256] = temp2;
    writeSector(fp,FAT,temp1/256);
}
/****************************************************************************/

int checkName(char* name, int *curdir, int* index,FILE *fp)
{
    int flag = 0,i;
    while(1)
    {
       for (i=0;i<DEPS;i++)
       {
            if ((buf.dirStruct_buffer.dir_array[i].dir_attributes&16)==16)
            {
                 /* name has been found in current directory,
                                    return zero and index of the name*/
                 if (strcmpi(buf.dirStruct_buffer.dir_array[i].name,name)==0)
                 {
                    *index = i;
                    return 0;
                 }
            }
            //an invalid location has been found
            else if (!flag)
            {
                 //it is the first invalid index found, to be returned later
                 flag = 1;
                 *index = i;
            }
       }
    
       readSector(fp,FAT,*curdir/FEPS);
       if (buf.FAT_buffer[*curdir%FEPS]!=-1)
       {
             *curdir = buf.FAT_buffer[*curdir%FEPS];
             readSector(fp,dirStruct,*curdir);
       }
       else
       {
              readSector(fp,dirStruct,*curdir);
              //there was no collision
              if (flag) //an empty index was found, return its index
                return 1;
              *index = -1; //no empty location found
              return -1;
       }
    }
}
/****************************************************************************/
/* modifies attributes of a file */
char attr(int dirctry)
{
     char ans, result;
     result = result | 16;        /*set valid bit*/
     
     //set directory bit if specified argument is a directory
     if (dirctry == 1)
       result = result | 8;
     else
       result = result & 23;      /*reset otherwise*/

     printf("\nEnter attributes! \nType 'y' for enabling an attribute\n");
     fflush(stdin);
     printf("\n\n");
     printf("hidden?\n");
     scanf("%c",&ans);
     if(ans=='y' || ans=='Y')
       result = result|4;         /*set hidden bit*/
     else
       result = result & 27;      /*clear hidden bit*/
     fflush(stdin);

     printf("archive?\n");
     scanf("%c",&ans);
     if(ans=='y' || ans=='Y')
       result = result|2;         /*set archive bit*/
     else
       result = result & 29;      /*clear archive bit*/
     fflush(stdin);
  
     printf("read-only?\n");
     scanf("%c",&ans);
     if(ans=='y' || ans=='Y')      /*set read-only bit*/
       result = result|1;
     else
       result = result & 30;       /*reset read-only bit*/
     fflush(stdin);
  
     return result;
}

/****************************************************************************/
void makeEntry(int fl, char* name, char dir, int size, int tempfb)
{
     //setting all attributes of directory entry in free location
     if (dir=='D')
        buf.dirStruct_buffer.dir_array[fl].dir_attributes = 24;    //00011000
     else
        buf.dirStruct_buffer.dir_array[fl].dir_attributes = 16;    //00010000
        
     buf.dirStruct_buffer.dir_array[fl].size = size;
       
     if (dir=='D')    
        buf.dirStruct_buffer.dir_array[fl].size_on_disk = 0;
     else if (dir=='F')
        buf.dirStruct_buffer.dir_array[fl].size_on_disk = 1;
        
     buf.dirStruct_buffer.dir_array[fl].starting_address = tempfb;
     strcpy(buf.dirStruct_buffer.dir_array[fl].name,name);
     buf.dirStruct_buffer.dir_array[fl].date_created = time(0);
     buf.dirStruct_buffer.dir_array[fl].date_modified = time(0); 
}
/****************************************************************************/

void update(FILE *fp,int curdir,int updateSizeOnDisk,int sz,int set_date)
{
    int isRoot,parent,child,i,j;
    int flag;
    isRoot=0;
    readSector(fp,mbr,0);
    parent = buf.mbr_buffer.start_of_root;
    
    while(parent!=-1)
    {
      if(curdir==parent)
        isRoot=1;
      readSector(fp,FAT,parent/FEPS);
      parent = buf.FAT_buffer[parent%FEPS];
    }
    //parent is not root
    if (isRoot==0)
    {
       j=tos;
       //for every entry in family stack
       while(j!=0)
       {
         flag = 0;
         child = family_stack[j];    //call that entry child
         parent = family_stack[j-1]; //and its previous entry parent
         j--;
         readSector(fp,dirStruct,parent);
         //find entry of child in directory structure of parent
         while(1)
         {
            for(i=0; i<DEPS; i++)
            {
               //once found, update size and date modified of child
              if(buf.dirStruct_buffer.dir_array[i].starting_address == child)
              {                                           
               buf.dirStruct_buffer.dir_array[i].size_on_disk+=updateSizeOnDisk;
               buf.dirStruct_buffer.dir_array[i].size+=sz;
               if(set_date)
                 buf.dirStruct_buffer.dir_array[i].date_modified = time(0);
               writeSector(fp,dirStruct,parent);
               flag=1;
               break;
              }
             }
             if(flag)
               break;
             else
             {
               //if not, look in next directory structure
               readSector(fp,FAT,parent/FEPS);
               parent = buf.FAT_buffer[parent%FEPS];
               readSector(fp,dirStruct,parent);
             }
         } //end of inner while
         writeSector(fp,dirStruct,parent);
       } //end of outer while
    } //end of if
}
/****************************************************************************/
    
void display_size(FILE *fp)
{
     int i,temp;
     int size = 0;
     readSector(fp,mbr,0);
        temp = buf.mbr_buffer.start_of_root;
     
     while(1)
     {
          readSector(fp,dirStruct,temp);
          for(i=0;i<DEPS;i++)
          {
            /*valid entry */
           if((buf.dirStruct_buffer.dir_array[i].dir_attributes & 16) == 16)
            size = size + (buf.dirStruct_buffer.dir_array[i].size_on_disk*1024);
          }
                   
          readSector(fp,FAT,temp/FEPS);
          if(buf.FAT_buffer[temp%FEPS]==-1)
            break;
          temp = buf.FAT_buffer[temp%FEPS];      
     }
       
     printf("\nUsed Disk space = %f",(((float)size/(float)SIZE_OF_DISK)*100));
     printf(" %%");
     printf("\n\nFree Disk space = %f",
                                  100-(((float)size/(float)SIZE_OF_DISK)*100));
     printf(" %%");
}

/**************************************************************************/

void list_contents(int curdir, FILE *fp)
{
     char ans;
     int temp,i;
     char flag;
     char set;
     char hidden;
     
     flag = 'f';
     hidden = 'f';
     
     /*Determine whether there are any hidden files or folders*/
     while(1)
      {
        readSector(fp,dirStruct,curdir);
        for(i=0;i<DEPS;i++)
         {
             if((buf.dirStruct_buffer.dir_array[i].dir_attributes&16)==16) 
              {
                if((buf.dirStruct_buffer.dir_array[i].dir_attributes&4)==4)
                 {
                   hidden = 't';
                   break;
                 }
              }
         }
         
         if(hidden)
           break;
           
         readSector(fp,FAT,curdir/FEPS);   
         if(buf.FAT_buffer[curdir%FEPS]!=-1)
            curdir = buf.FAT_buffer[curdir%FEPS];
         else
            break;
      }
     
     
     if(hidden=='t')  /*there are hidden files and folders*/
       {
            printf("\nThis folder contains some hidden files or folders");
            printf("\nDo you want them to be displayed?\n");
            printf("\nType 'y' for yes\n");
            scanf("%c",&ans);
            fflush(stdin);
            if(!((ans=='y')||(ans=='Y')))
              hidden = 'f';  //hidden files and folders are not to be displayed
      }  
     
     
     while (1)
     {
         readSector(fp,dirStruct,curdir);
         for(i=0;i<DEPS;i++) //for every entry in the directory structure
         {
            if((buf.dirStruct_buffer.dir_array[i].dir_attributes&16)==16)
            //if it is valid
            {
              set = 'f';
              printf("\n\n");
              
              flag = 't';  
              //indicates that folder is not empty
              
              if(((buf.dirStruct_buffer.dir_array[i].dir_attributes&4)==4)
                                                                   &&(!hidden))
                continue;   
                /*if it is hidden and user does not want to print hidden 
                    objects,then repeat for next entry else print all details*/
                    
              printf("\n\n NAME          : %s",
                           buf.dirStruct_buffer.dir_array[i].name);
              if((buf.dirStruct_buffer.dir_array[i].dir_attributes&8)==8)
                printf("\n TYPE          : FOLDER  ");
              else
                printf("\n TYPE          : TEXT FILE  ");
                    
              printf("\n DATE CREATED  : %s", 
                       ctime(&buf.dirStruct_buffer.dir_array[i].date_created));
              printf(" DATE MODIFIED : %s", 
                       ctime(&buf.dirStruct_buffer.dir_array[i].date_modified));
              printf(" SIZE ON DISK  : %d KB",
                       buf.dirStruct_buffer.dir_array[i].size_on_disk);
              printf("\n SIZE          : %d Bytes",
                         buf.dirStruct_buffer.dir_array[i].size);
              printf("\n ATTRIBUTES    :");
                    
              if((buf.dirStruct_buffer.dir_array[i].dir_attributes&4)==4)
              {
                  printf(" HIDDEN  ");
                  set = 't';
              }
              if((buf.dirStruct_buffer.dir_array[i].dir_attributes&2)==2)
              {
                 printf(" ARCHIVE  ");
                 set = 't';
              }
              if((buf.dirStruct_buffer.dir_array[i].dir_attributes&1)==1)
              {
                  printf(" READ-ONLY  ");
                  set = 't';
              }
              if((buf.dirStruct_buffer.dir_array[i].dir_attributes&32)==32)
              {
                printf(" PASSWORD PROTECTED  \n");
                set = 't';
              }
              
              //if file has no attributes set, then print no attributes
              if(set=='f')
                 printf(" NO ATTRIBUTE IS SET");
            }
         }
         readSector(fp,FAT,curdir/FEPS);   
         if(buf.FAT_buffer[curdir%FEPS]!=-1)
            curdir = buf.FAT_buffer[curdir%FEPS];
         else
            break;
     }
     
     if(flag=='f')  //folder is empty
        printf("\nNo files\\folders found..\n");
     printf("\n\n");
}

/****************************************************************************/

/* cut a file\\folder by storing its directory-
   entry in a temporary location
*/
void cut(struct dir_entry* temp, char name[], int curdir, FILE* fp)
{
     int check,index,flag;
     
     /* If directory entry is an entry from recycle bin,
        then do not call update(....)
     */
     readSector(fp,mbr,0);
     if (curdir == buf.mbr_buffer.start_of_recycle_bin)
       flag = 1;
     else 
       flag = 0;
       
     readSector(fp,dirStruct,curdir);
     check = checkName(name,&curdir,&index,fp);
     
     if (check==0)
     {            
                 /* copy directory-entry from disk in some temporary location
                    and invalidate directory entry of the disk
                 */
                 strcpy(temp->name,buf.dirStruct_buffer.dir_array[index].name);
                 temp->dir_attributes = 
                          buf.dirStruct_buffer.dir_array[index].dir_attributes;
                 
                 temp->starting_address =
                        buf.dirStruct_buffer.dir_array[index].starting_address;
                 
                 temp->size =  buf.dirStruct_buffer.dir_array[index].size;
                 
                 temp->size_on_disk =
                            buf.dirStruct_buffer.dir_array[index].size_on_disk;
                 
                 temp->date_created =
                            buf.dirStruct_buffer.dir_array[index].date_created;
                 
                 temp->date_modified =
                           buf.dirStruct_buffer.dir_array[index].date_modified;
                 
                  buf.dirStruct_buffer.dir_array[index].dir_attributes = 
                      buf.dirStruct_buffer.dir_array[index].dir_attributes & 0;
                  writeSector(fp,dirStruct,curdir);
                  if (flag==0)
                     update(fp,curdir,-(temp->size_on_disk),-(temp->size),1);
     }
     else
         printf("This file\\folder does not exist!\n");
}

/****************************************************************************/
/* copy a file\\folder by storing its directory-
   entry in a temporary location
*/
 void copy(struct dir_entry* temp, char name[], int curdir, FILE *fp)
{
     int check,index;
     readSector(fp,dirStruct,curdir);
     check = checkName(name,&curdir,&index,fp);
     
     if (check==0)
     {
                 /* copy directory-entry from disk in some temporary location
                    but do not invalidate directory entry of the disk
                 */
                 strcpy(temp->name,buf.dirStruct_buffer.dir_array[index].name);
                 
                 temp->dir_attributes =
                          buf.dirStruct_buffer.dir_array[index].dir_attributes;
                 
                 temp->starting_address =
                        buf.dirStruct_buffer.dir_array[index].starting_address;
                 
                 temp->size =  buf.dirStruct_buffer.dir_array[index].size;
                 
                 temp->size_on_disk =
                            buf.dirStruct_buffer.dir_array[index].size_on_disk;
                 
                 temp->date_created =
                            buf.dirStruct_buffer.dir_array[index].date_created;
                 
                 temp->date_modified =
                           buf.dirStruct_buffer.dir_array[index].date_modified;
     }
     else
         printf("This file\\folder does not exist!\n");
}

/****************************************************************************/

/* paste directory entry from the temporary location
   on the disk
*/
void paste(struct dir_entry *temp, int* curdir,int* index, FILE *fp)
{
   char name[11];
   int check,flag;
   int i,temp1,tempfb;
   
   readSector(fp,mbr,0);
   tempfb = buf.mbr_buffer.start_of_first_fb;
   
   if (*curdir==buf.mbr_buffer.start_of_recycle_bin)
     flag = 1;
   else 
     flag = 0;
     
   strcpy(name,temp->name);
   
   /* determine if file with the same name already exists
      in current directory or not
   */
   readSector(fp,dirStruct,*curdir);
   check = checkName(name, curdir, index, fp);
   
   /* if file\\folder with same name already exists in recycle-bin,
      then also send it to bin
   */
   if(check == 0)
   {
       if (flag==0)
       {
          printf("\n File\\folder with this name already exists!!");
          return;
       }
       else
           check = checkName("garbage",curdir,index,fp);
   }
   
   /* If the file\\folder that has been cut or copied 
      earlier has been deleted.....
      Hence, it cannot be pasted now
   */
   if(temp->starting_address == -1)
   {
          printf("\nCannot read from source file or disk");
          return;
   }
   
   if(check == 1)
   {
           readSector(fp,dirStruct,*curdir);
           strcpy(buf.dirStruct_buffer.dir_array[*index].name,temp->name);
           
           buf.dirStruct_buffer.dir_array[*index].dir_attributes = 
                                                          temp->dir_attributes;
           
           buf.dirStruct_buffer.dir_array[*index].starting_address =
                                                        temp->starting_address;
           
           buf.dirStruct_buffer.dir_array[*index].size = temp->size;           
           
           buf.dirStruct_buffer.dir_array[*index].size_on_disk =
                                                            temp->size_on_disk;    
           
           buf.dirStruct_buffer.dir_array[*index].date_created =
                                                            temp->date_created;
           
           buf.dirStruct_buffer.dir_array[*index].date_modified = time(0);
           writeSector(fp,dirStruct,*curdir);
    }
    else /*extra struct is to be allocated*/
    {
         /* check if disk is full */
         if(tempfb == -1)
           {
              printf("\n\nDISK IS FULL!!\n");
              return;
           }
         temp1 = tempfb;
         readSector(fp,FAT,temp1/FEPS);
         tempfb = buf.FAT_buffer[temp1%FEPS];
         
         readSector(fp,FAT,*curdir/FEPS);
         buf.FAT_buffer[*curdir%FEPS] = temp1;
         writeSector(fp,FAT,*curdir/FEPS);
         
         readSector(fp,FAT,temp1/FEPS);
         buf.FAT_buffer[temp1%FEPS] = -1;
         writeSector(fp,FAT,temp1/FEPS);
         
         readSector(fp,mbr,0);
         buf.mbr_buffer.start_of_first_fb = tempfb;
         writeSector(fp,mbr,0);
         
         readSector(fp,dirStruct,*curdir);
         for (i=0;i<DEPS;i++)
             buf.dirStruct_buffer.dir_array[i].dir_attributes = 
                   buf.dirStruct_buffer.dir_array[i].dir_attributes & 0;
         if (flag==1)
             for (i=1;i<DEPS;i=i+2)
                   buf.dirStruct_buffer.dir_array[i].size = -1;
                   
         *index = 0;
         strcpy(buf.dirStruct_buffer.dir_array[*index].name,temp->name);
         
         buf.dirStruct_buffer.dir_array[*index].dir_attributes =
                                                          temp->dir_attributes;
         
         buf.dirStruct_buffer.dir_array[*index].starting_address =
                                                        temp->starting_address;
         
         buf.dirStruct_buffer.dir_array[*index].size = temp->size;           
         
         buf.dirStruct_buffer.dir_array[*index].size_on_disk =
                                                            temp->size_on_disk;    
         
         buf.dirStruct_buffer.dir_array[*index].date_created =
                                                            temp->date_created;
         
         buf.dirStruct_buffer.dir_array[*index].date_modified = time(0);
         writeSector(fp,dirStruct,*curdir);
                   
         writeSector(fp,dirStruct,temp1);
         if (!flag)                    //this is not a recycle bin operation
            update(fp,*curdir,0,0,1);
    }
    if (!flag)                        //this is a not a recycle bin operation
    {
         update(fp,*curdir,temp->size_on_disk,temp->size,1);
    }
}
/****************************************************************************/

/* Determines if current directory is read-only or not
   return 0: if read-only
   return 1: otherwise
*/
int read_only(int curdir, FILE *fp)
{
    int parentdir;
    int i;
    int temp;
    parentdir = family_stack[tos-1];
    readSector(fp,mbr,0);
    
    /* If current directory is actually root, then it cannot be read-only */
    if(curdir == buf.mbr_buffer.start_of_root)  
       return 1;
    
    /* otherwise determine whether current directory is read-only or not,
       by checking read-only bit of current directory
       in its parent directory 
    */
    temp = parentdir;
    while(1)
    {
            readSector(fp,dirStruct,temp);
     
            for(i=0;i<DEPS;i++)
                if(buf.dirStruct_buffer.dir_array[i].starting_address==curdir)
                {
                  //read only
                  if(((buf.dirStruct_buffer.dir_array[i].dir_attributes)&1)==1)
                     return 0;
                  else 
                     return 1;
                }
                readSector(fp,FAT,temp/FEPS);
                temp = buf.FAT_buffer[temp%FEPS];
     }  
}
/*****************************************************************************/

void modify_attr(char *name, int curdir, FILE *fp)
{
    int check,temp,index;
    char c;
     
    readSector(fp,dirStruct,curdir);;
     
     /*check for any collision and also give first free location in fl*/
     check = checkName(name,&curdir,&index,fp);
     if (check==0)
     {
         c = buf.dirStruct_buffer.dir_array[index].dir_attributes;
         if (strstr(name,".txt")!=NULL)
            buf.dirStruct_buffer.dir_array[index].dir_attributes = attr(0);
         else
            buf.dirStruct_buffer.dir_array[index].dir_attributes = attr(1);
         if ((c&32)==32)
         /*if file was initially passwork protected,
                 then it should still be password protected*/
            buf.dirStruct_buffer.dir_array[index].dir_attributes = 
               buf.dirStruct_buffer.dir_array[index].dir_attributes | 32;
     }
    
    writeSector(fp,dirStruct,curdir);
}
/****************************************************************************/

/*delete specified file\\folder from the system*/
void del_f(char* name, int curdir, struct dir_entry *tmpr, FILE *fp)
{
     int i,check,index,temp,temp1,temp2,size,size_on_disk,flag,startdir;
     char c;
     startdir = curdir;
     flag = 0;
     readSector(fp,mbr,0);
     
     /* if current directory is actually recycle-bin */
     if (curdir==buf.mbr_buffer.start_of_recycle_bin)
        flag = 1;
     
     readSector(fp,dirStruct,curdir);
     check = checkName(name,&curdir,&index,fp);
     
     /* if file\folder exists in current directory */
     if (check==0)
     {
                  /* then invalidate directory entry corresponding to
                     specified file\folder
                  */
                 c = buf.dirStruct_buffer.dir_array[index].dir_attributes;
                 
                 buf.dirStruct_buffer.dir_array[index].dir_attributes = 
                    buf.dirStruct_buffer.dir_array[index].dir_attributes & 0;
                 
                 size = buf.dirStruct_buffer.dir_array[index].size;  
                 
                 size_on_disk = 
                           buf.dirStruct_buffer.dir_array[index].size_on_disk;  
                 
                 writeSector(fp,dirStruct,curdir);
                 
                 temp = buf.dirStruct_buffer.dir_array[index].starting_address;
                 /* and if that file\folder has been cut or copied before,
                    then invalidate temporary location also....
                    for maintaining consistency with paste
                 */
                 if(temp == tmpr->starting_address)
                    tmpr->starting_address = -1;
                 remove_struct(startdir,curdir,fp);
                 readSector(fp,dirStruct,temp);
                 
                 if (((c&8)!=8)&&(flag==0))                                                 
                    update(fp,curdir,-(size_on_disk),-(size),1);
                 else  if ((c&8)==8)        /* if directory is to be deleted */                                              
                 {
                     while (1)
                     {
                           readSector(fp,dirStruct,temp);
                           
                           /* Now, delete all files and folders also
                              from that directory
                           */
                           for (i=0;i<DEPS;i++)
                           if((buf.dirStruct_buffer.dir_array[i].
                                                        dir_attributes&16)==16)
                           {
                               del_f(buf.dirStruct_buffer.dir_array[i].name,
                                                                 temp,tmpr,fp);
                               readSector(fp,dirStruct,temp);
                           }
                           readSector(fp,dirStruct,curdir);
                           size = buf.dirStruct_buffer.dir_array[index].size;
                           
                           /* Do not call update(....) if file\\folder
                              is to deleted from recycle-bin
                           */
                           if (flag==0)
                              update(fp,curdir,-(size_on_disk),-size,1);  
                           readSector(fp,FAT,temp/FEPS);
                           if (buf.FAT_buffer[temp%FEPS]==-1)
                              break;
                           else
                               temp = buf.FAT_buffer[temp%FEPS];
                     }
                 }
                 readSector(fp,mbr,0);
                 temp1 = buf.mbr_buffer.start_of_first_fb;
                 temp2 = temp;
                 
                 /*updating FAT and mbr*/    
                 readSector(fp,FAT,temp2/FEPS);
                 while (buf.FAT_buffer[temp2%FEPS]!=-1)
                 {
                           temp2 = buf.FAT_buffer[temp2%FEPS];
                           readSector(fp,FAT,temp2/FEPS);
                 }
                 buf.FAT_buffer[temp2%FEPS] = temp1;
                 writeSector(fp,FAT,temp2/FEPS);
                     
                 readSector(fp,mbr,0);
                 buf.mbr_buffer.start_of_first_fb = temp;
                 writeSector(fp,mbr,0);
     
     }
     else printf("File/Folder not found!\n");
}

/****************************************************************************/









/****************************************************************************/


int createDir(int curdir, char name[], FILE *fp)
{
    int check,index,tempfb,temp,i;
    char count = '1';
    char tempName[10];
    
    if(strcmpi(name,"root")==0)
    {
      printf("\nroot directory not allowed!!");
      return curdir;
    }
    
    readSector(fp,mbr,0);
    tempfb = buf.mbr_buffer.start_of_first_fb;
     /* check if disk is full */
     if(tempfb == -1)
      {
        printf("\n\nDISK IS FULL!!\n");
        return;
      }
    
    //child directory is to be created in curdir
    readSector(fp,dirStruct,curdir);
    check = checkName(name,&curdir,&index,fp);
    //there has been a collision
    if (check==0)
    {
         //but with a system assigned name
         if (strstr(name,"NewFolder")!=NULL)
         {
             while (!check)
             {     //so system assigns it another name
                   for (check=0;check<9;check++)
                     tempName[check] = name[check];
                   tempName[9]= count;
                   tempName[10] = '\0';
                   count++;
                   
                   check = checkName(tempName,&curdir,&index,fp);
             }
             name = tempName;
         }
         //collision is not with system assigned name, cannot create directory    
         else
          {
            printf("Directory already exists!!");
            return curdir;
          }
    }
    
    //there has been no collision & there is space in parent directory struct
    if (check==1)
    {    
         makeEntry(index,name,'D',0,tempfb);
         writeSector(fp,dirStruct,curdir);    
    }
    //there is no collision & there is no space in current directory structure
    //an extra sector must be allocated to parent
    else
    {
         /*check if disk is full*/
         readSector(fp,FAT,tempfb/256);
         /*atleast two sectors are needed...
           one for extra sector and one for new directory 
         */
         if(buf.FAT_buffer[tempfb%256] == -1)
           {
             printf("\n\nDISK IS FULL!!\n");
             return curdir;
           }
         //allocating an extra sector, changing FAT and mbr
         temp = tempfb;
         readSector(fp,FAT,temp/FEPS);
         tempfb = buf.FAT_buffer[temp%FEPS];
         
         readSector(fp,FAT,curdir/FEPS);
         buf.FAT_buffer[curdir%FEPS] = temp;
         writeSector(fp,FAT,curdir/FEPS);
         
         readSector(fp,FAT,temp/FEPS);
         buf.FAT_buffer[temp%FEPS] = -1;
         writeSector(fp,FAT,temp/FEPS);
         
         readSector(fp,mbr,0);
         buf.mbr_buffer.start_of_first_fb = tempfb;
         writeSector(fp,mbr,0);
         
         //invalidating all entries in extra sector
         readSector(fp,dirStruct,temp);
         for (i=0;i<DEPS;i++)
             buf.dirStruct_buffer.dir_array[i].dir_attributes = 
                   buf.dirStruct_buffer.dir_array[i].dir_attributes & 0;
         //make an entry for child directory in extra sector
         makeEntry(0,name,'D',0,tempfb);
                   
         writeSector(fp,dirStruct,temp);
         update(fp,curdir,0,0,1);
    }
    
    update(fp,curdir,0,0,1);
    
    //allocating a sector to child directory
    //invalidating sector
    readSector(fp,dirStruct,tempfb);
    for(i=0;i<DEPS;i++)
       buf.dirStruct_buffer.dir_array[i].dir_attributes = 
            buf.dirStruct_buffer.dir_array[i].dir_attributes & 0;
    writeSector(fp,dirStruct,tempfb);
    
    //changing FAT and mbr
    temp = tempfb;
    readSector(fp,FAT,temp/FEPS);
    tempfb = buf.FAT_buffer[temp%FEPS];
    
    readSector(fp,mbr,0);
    buf.mbr_buffer.start_of_first_fb = tempfb;
    writeSector(fp,mbr,0);
    
    readSector(fp,FAT,temp/FEPS);
    buf.FAT_buffer[temp%FEPS] = -1;
    writeSector(fp,FAT,temp/FEPS); 
    curdir = temp;
    push(curdir);
    return curdir;    
}
/****************************************************************************/

/*cd path-to-directory 
  changes current directory to the path that is specified as an argument
  Ex- root> cd dir1\dir2
      root\dir1\dir2>
*/
int changePath(char path[], int curdir, FILE *fp)
{
    char *temp;
    /*tokenize path taking '\' as separator
      Hence, each token will be a directory name 
    */
    temp = strtok(path,"\\");
    
    /*change current directory to first token*/
    curdir = changeDirectory(temp,curdir,fp); 
    
    /*while there are more tokens, 
      change current directory from its 
      previous value to this new token...... 
    */
    while(temp!=NULL)
    {
         temp = strtok(NULL,"\\");
         if(temp == NULL)
           return curdir;
         curdir = changeDirectory(temp,curdir,fp);
    } 
    /*return value of current directory corresponding to the last token*/
    return curdir;
}
/****************************************************************************/
/* cd Directory-name
   - changes current directory to the user-specified directory ,
     if it is there in current directory......
   It is a helper function to changePath
*/

int changeDirectory(char *name, int curdir, FILE *fp)
{
    int i;
    while(1)
    {
            /* read sector corresponding to current directory */
            readSector(fp,dirStruct,curdir);
            
            for(i=0;i<DEPS;i++)
            {
              /* if the entry in current directory's sector
                 corresponds to a 'directory' and its name
                 matches to that specified by the user */
              if((buf.dirStruct_buffer.dir_array[i].dir_attributes&8)==8)
              if(strcmpi(buf.dirStruct_buffer.dir_array[i].name, name)==0)
              {
                 /*then, change current directory to this diectory*/
                 curdir = buf.dirStruct_buffer.dir_array[i].starting_address;
                 
                 /*and push it in family stack for traversal*/
                 push(curdir);
                 return curdir;
              }
            }
            /* read FAT sector corresponding to curdir entry */
            readSector(fp,FAT,curdir/FEPS);
            
            /* if name specified does not match any entry in curdir sector
               and there are no more sectors to be searched
               corresponding to curdir (this was the last sector),
               then do not change current directory and simply return
             */ 
            if(buf.FAT_buffer[curdir%FEPS]==-1)
            {
               printf("No such directory exists.. \n");
               return curdir;
            }
           
            /* if name specified by the user does not 
               match any entry of the read sector,
               but current directory spans more sectors
               (this was not the last sector),
               then determine further spanned sector from FAT and change
               curdir to this new value, and repeat the whole process......
            */
            curdir = buf.FAT_buffer[curdir%FEPS];
    }
}

/****************************************************************************/
/* cd.. 
   change current directory to its parent
   Ex- root\dir1\dir2> cd..
       root\dir1> cd..
       root>
*/
int cd_parent(FILE* fp, int curdir)
{
    readSector(fp,mbr,0);
    /*If current directory is already a root,
      then do not change current directory
      because root has no parent
    */
    if(curdir == buf.mbr_buffer.start_of_root)
       return curdir;
       
    /* change current directory to its parent
       by popping from family_stack 
    */
    curdir = pop();
    return curdir;
}

/****************************************************************************/

/*change current directory to root
  i.e. sector No. 129
  Ex- root\dir1\dir2> cd\
      root>
*/
int cd_root(FILE *fp,int curdir)
{
    /* read mbr to determine sector no. corresponding to root */
    readSector(fp,mbr,0);
    curdir = buf.mbr_buffer.start_of_root;
    tos = 0;
    return curdir;
}

/****************************************************************************/










/****************************************************************************/
/*Creating file*/

void createFile(char *name,int curdir, FILE *fp)
{
     int i,j,index,temp,tmpr,tempfb,check,size,sizeOnDisk,flag;
     char count = '1';
     char tempName[11];
     flag = 0;
     
     /* check if disk is full */
     readSector(fp,mbr,0);
     tempfb = buf.mbr_buffer.start_of_first_fb;
     if(tempfb == -1)
       {
         printf("\n\nDISK IS FULL!!\n");
         return;
       }
     
     readSector(fp,dirStruct,curdir);
     /*check for any collision and also give first free location in index*/
     check = checkName(name,&curdir,&index,fp); 
     
     //there has been a collison 
     if(check==0)
     {
          //but with a system assigned name
          if (strstr(name,"Newf")!=NULL)
          {
             //so system assigns it another unique name
             while (!check)
             { 
                   for (check=0;check<4;check++)
                     tempName[check] = name[check];
                   tempName[4]= count;
                   tempName[5] = '.';
                   tempName[6] = 't';
                   tempName[7] = 'x';
                   tempName[8] = 't';
                   tempName[9] = '\0';
                   count++;
                   
                   check = checkName(tempName,&curdir,&index,fp);
             }
             name = tempName;
         }
         //collision is with user given name, file cannot be created    
         else
         {
             printf("File with the same name already exists");
             printf("\nin the current directory!!");
             return;
         }
     }
     //there is no collison
     //so space is allocated to file
     readSector(fp,data,tempfb);
     printf("\nEnter the data");
     printf("\nPress Esc Key for end-of-file\n\n");
     for(i=SIZE_OF_PASSWORD;i<SIZE_OF_SECTOR;i++)
       {
         buf.data_buffer[i] = getch();
         if(buf.data_buffer[i] == 27)
          {
            buf.data_buffer[i] = '\0';
            flag = 1;
            break;
          }
         if(buf.data_buffer[i] == 13)
           putchar('\n');
         else
           putchar(buf.data_buffer[i]);
       }
     //gets(&buf.data_buffer[SIZE_OF_PASSWORD]);
     fflush(stdin);
     
     //user begins to enter data 7th byte onwards
     //password is given a default value of "******"
     for (i=0;i<SIZE_OF_PASSWORD;i++)
         buf.data_buffer[i] = '*';
     writeSector(fp,data,tempfb); 

     size = strlen(buf.data_buffer)-SIZE_OF_PASSWORD;
     //6 bytes corresponding to password are subracted
     
     sizeOnDisk = 1;
     
     if(check==1) 
     {
              //there is space in parent directory structure
              readSector(fp,dirStruct,curdir);
              makeEntry(index,name,'F',size,tempfb);  
              writeSector(fp,dirStruct,curdir);
     }
     else
     {
              //check = -1,there is no space,so allocate an extra structure
              temp = tempfb;
              /*check if disk is full*/
              readSector(fp,FAT,tempfb/256);
              /*atleast two sectors are needed
                one for extra sector and one for the new file
              */
              if(buf.FAT_buffer[tempfb%256] == -1)
               {
                  printf("\n\nDISK IS FULL\n");
                  return;
               }
              readSector(fp,FAT,temp/FEPS);
              tempfb = buf.FAT_buffer[temp%FEPS];
           
              readSector(fp,FAT,curdir/FEPS);
              buf.FAT_buffer[curdir%FEPS] = temp;
              writeSector(fp,FAT,curdir/FEPS);
           
              readSector(fp,FAT,temp/FEPS);
              buf.FAT_buffer[temp%FEPS] = -1;
              writeSector(fp,FAT,temp/FEPS);
           
              readSector(fp,mbr,0);
              buf.mbr_buffer.start_of_first_fb = tempfb;
              writeSector(fp,mbr,0);
           
              readSector(fp,dirStruct,tempfb);
              for (i=0;i<DEPS;i++)
                buf.dirStruct_buffer.dir_array[i].dir_attributes = 
                   buf.dirStruct_buffer.dir_array[i].dir_attributes & 0;

              makeEntry(0,name,'F',size,tempfb);
           
              writeSector(fp,dirStruct,temp);
              update(fp,curdir,1,0,1);
     }
     //changing mbr and FAT for allocation of space to file data   
     temp = tempfb;
     readSector(fp,FAT,temp/FEPS);
     tempfb = buf.FAT_buffer[temp%FEPS];
     
     readSector(fp,mbr,0);
     buf.mbr_buffer.start_of_first_fb = tempfb;
     writeSector(fp,mbr,0);
     
     readSector(fp,FAT,temp/FEPS);
     buf.FAT_buffer[temp%FEPS] = -1;
     writeSector(fp,FAT,temp/FEPS);
     
     
     
      while(!flag)   /*more data is there....*/
       {
        tmpr = temp;
        readSector(fp,mbr,0);
        temp= buf.mbr_buffer.start_of_first_fb;
        /*check if disk is full*/
        if(temp == -1)
          {
            printf("\n\nDISK IS FULL!!\n");
            return;
          }
        
        readSector(fp,FAT,temp/256);
        tempfb = buf.FAT_buffer[temp%256];
        
        readSector(fp,mbr,0);
        buf.mbr_buffer.start_of_first_fb = tempfb;
        writeSector(fp,mbr,0);
        
        readSector(fp,FAT,tmpr/256);
        buf.FAT_buffer[tmpr%256] = temp;
        writeSector(fp,FAT,tmpr/256);
        
        readSector(fp,FAT,temp/256);
        buf.FAT_buffer[temp%256] = -1;
        writeSector(fp,FAT,temp/256);
        
        for(i=0;i<SIZE_OF_SECTOR;i++)
         {
           buf.data_buffer[i] = getch();
           if(buf.data_buffer[i] == 27)
            {
              buf.data_buffer[i] = '\0';
              flag = 1;
              break;
             }
           if(buf.data_buffer[i] == 13)
              putchar('\n');
           else
              putchar(buf.data_buffer[i]);
          }     
        
       size = size + strlen(buf.data_buffer);
       sizeOnDisk++;
       writeSector(fp,data,temp);
     }  
     
     update(fp,curdir,sizeOnDisk,size,1);
     
     /* updating directory entry of the file */
     readSector(fp,dirStruct,curdir);
     buf.dirStruct_buffer.dir_array[index].date_modified = time(0);
     buf.dirStruct_buffer.dir_array[index].size = size;
     buf.dirStruct_buffer.dir_array[index].size_on_disk = sizeOnDisk;
     writeSector(fp,dirStruct,curdir);
}

/****************************************************************************/


void rename_file(char *name, char *new_name, int curdir, FILE *fp)
{
    int i,j;
    int temp,check,index,dummy;
    
    if((strstr(name,".txt")!=NULL)&&(strstr(new_name,".txt")==NULL))
    {
          printf("\nSecond argument must be a file!!");
          return;
    }
    
    if((strstr(name,".txt")==NULL)&&(strstr(new_name,".txt")!=NULL))
    {
        printf("\nSecond argument must be a directory");
        return;
    }
    readSector(fp,dirStruct,curdir);
            
    check = checkName(name,&curdir,&index,fp);
    if (check==0)
    {
         //check if new name given by user has already been used                     
         check = checkName(new_name,&curdir,&dummy,fp);
         if (check==0)
         {
             printf("\nFile\\Folder with the same name already exists...\n");
             return;
         }
         //if not, then change name
         strcpy(buf.dirStruct_buffer.dir_array[index].name, new_name);
         writeSector(fp,dirStruct,curdir);
         printf("\n File has been renamed successfully..");
    }
    else
       printf("\n File\\Folder to be re-named not found.. ");
                   
}
/****************************************************************************/
/* determine whether a particular file can be accessed by the user or not
   return 1: if it can be accessed
   and return 0: otherwise
*/
int accessFile(char* name, int curdir,int* temp, FILE *fp)
{
    int check,index,flag,chances,i;
    char a;
    
    if (strstr(name,".txt")==NULL)
    {
         printf("Cannot edit directory\n");
         return 0;
    }
    
    readSector(fp,dirStruct,curdir);
    check = checkName(name,&curdir,&index,fp);
    
    /* if specified file does not exists in current directory */
    if (check)
    {
              printf("\nNo such file exists.....");
              return 0;
    }
    
    if ((buf.dirStruct_buffer.dir_array[index].dir_attributes &32)!=32)
         return 1;
         
    *temp = buf.dirStruct_buffer.dir_array[index].starting_address;
    chances = 3;
    readSector(fp,data,*temp);
    
    /* If file is password protected,
       then user has maximum three chances to open the file
       And if user is not able to enter correct password
       in three chances, then he cannot access the file
     */ 
    do
    {
        printf("\nEnter Password: ");
        check = 0;
        for (i=0;i<SIZE_OF_PASSWORD;i++)
        {
            a = getch();
            putchar('*');
            if (a!=buf.data_buffer[i])
              check = 1;
        }
        chances--;
        if (check==1)
          printf("\nIncorrect Password!\n");
        printf("\n");
    }while ((check==1)&&(chances>0));
     
    if (check==0)
         return 1;
    return 0;
}
            
/****************************************************************************/
// open and display contents of a file, if it exists in the current directory
void open_file(char *name, int curdir, FILE *fp)
{
     int check,index,temp1;
     int i,temp;
     char c;
     temp = curdir;
     check = 1;
     
     if(strstr(name,".txt")==NULL)
     {
         printf("\nCannot open directory %s",name);
         return;
     }
    
    readSector(fp,dirStruct,curdir);
    check = checkName(name,&curdir,&index,fp);
    /* curdir and index are sector no. and index corresponding
       to specified file-entry in current directory
    */
    temp1 = 0;
    
    /* if file exists in current directory */
    if(check==0)
    {
        temp = buf.dirStruct_buffer.dir_array[index].starting_address;
        /* if specified file is password protected */
        if ((buf.dirStruct_buffer.dir_array[index].dir_attributes&32)==32)
        {
           check = accessFile(name,curdir,&temp,fp);
           if (check==0)
           {
                       printf("Sorry! Cannot Access File!\n");
                       return;
           }
        }
        while (1)
        {
                readSector(fp,data,temp);
                for (i=0;i<SIZE_OF_SECTOR;i++)
                {
                    if (temp1 == 0)
                      i = SIZE_OF_PASSWORD;
                    if (buf.data_buffer[i]=='\0')
                      break;
                    if(buf.data_buffer[i] == 13)
                       printf("\n");
                    else
                       printf("%c",buf.data_buffer[i]);
                    temp1++;
                }
                
                readSector(fp,FAT,temp/FEPS);
                if (buf.FAT_buffer[temp%FEPS]==-1)
                  break;
                else
                  temp = buf.FAT_buffer[temp%FEPS];
        }
    }
   else
       printf("\n File not found.... "); 
}
/****************************************************************************/

/* Appends data in an already existing file */
void append(char *name, int curdir, FILE *fp)
{
     int i,j,k,size,size_on_disk,max_data;
     int temp,tmpr,temp1,tempfb,index,startdir;
     char ans;
     char contents[1024];
     int flag = 0;
     
     /* read sector corresponding to current directory and 
        check if specified file exists in current directory
     */
     readSector(fp,dirStruct,curdir);
     checkName(name, &curdir, &index, fp);
     /* Now, curdir & index are  sector no. and
        index corresponding to file-entry in current directory
     */
     
     /* if specified file is read-only, then user cannot 
        append any data, hence, simply return
     */
     if((buf.dirStruct_buffer.dir_array[index].dir_attributes & 1)==1)
        {
          printf("\nFile is read-only!!");
          printf("\nYou cannot append......");
          return;
        }
     
     /* otherwise */
     size = 0;
     size_on_disk = 0;
     
    /* moving to last sector of the file */
    temp = buf.dirStruct_buffer.dir_array[index].starting_address;
    startdir = temp;
    
    readSector(fp,FAT,temp/FEPS);
    while(buf.FAT_buffer[temp%FEPS]!=-1)
       temp = buf.FAT_buffer[temp%FEPS];
    
    readSector(fp,data,temp);
    /* Now, there are 2 cases -
       - if this last sector read is actually half spanned sector
       - if this last sector read is completely filled sector and a
         new sector is to be allocated now, to store more data.....
    */
    
    /* But how to determine that a particular sector is half-spanned or not?
       Maximum data that can be stored in a sector actually
       depends on whether it is a first sector,
       or some sector other than first sector
    */
       
    /* if this is actually first sector,
       then maximum data that can be stored is 1018 characters
       because first 6 bytes are reserved for storing password
    */
    if(temp == startdir)
       max_data = (SIZE_OF_SECTOR - SIZE_OF_PASSWORD);
       
    /*if it is some sector,but not the first sector,
      then maximum data that can be stored is actually SIZE_OF_SECTOR
      since no password is stored in this sector
    */
    else
       max_data = SIZE_OF_SECTOR;
    
     /* case 1 - it is half spanned sector  */   
    if(strlen(buf.data_buffer)<max_data)
    {
       /* copy already stored data from the last sector 
          into a temporary array- contents[]
       */
       for(k=0;k<SIZE_OF_SECTOR;k++)
       {
          if(buf.data_buffer[k]=='\0')
            break;
          contents[k] = buf.data_buffer[k];
       }
       
       /* appending new data in that half spanned sector */
       printf("\nEnter the data: ");
       printf("\nPress Esc Key for end-of-file\n\n");
       for(i=0;i<SIZE_OF_SECTOR;i++)
         {
           buf.data_buffer[i] = getch();
           if(buf.data_buffer[i] == 27)
            {
              buf.data_buffer[i] = '\0';
              flag = 1;
              break;
             }
           if(buf.data_buffer[i] == 13)
              putchar('\n');
           else
              putchar(buf.data_buffer[i]);
           } 
    
       j=0;
       for(i=k;i<SIZE_OF_SECTOR;i++)
       {
           if(buf.data_buffer[j]=='\0')
              break;
           contents[i]=buf.data_buffer[j];
           size++;
           j++;
       }
       contents[i] = '\0';    
       i=0;
       while(contents[i]!='\0')
         {
           buf.data_buffer[i] = contents[i];
           i++;
         }
       buf.data_buffer[i] = '\0';
       writeSector(fp,data,temp);
    }
    /* case 2 - a new sector is to be allocated */
    else
    {
        /* temp is sector no. corresponding to last sector of the file */
        tmpr = temp;
        readSector(fp,mbr,0);
        temp= buf.mbr_buffer.start_of_first_fb;
        /*check if disk is full*/
        if(temp == -1)
         {
           printf("\n\nDISK IS FULL!!\n");
           return;
         }
        
        /* updating FAT and mbr */
        readSector(fp,FAT,temp/FEPS);
        temp1 = buf.FAT_buffer[temp%FEPS];
        buf.FAT_buffer[temp%FEPS] = -1;
        writeSector(fp,FAT,temp/FEPS);
        
        readSector(fp,FAT,tmpr/FEPS);
        buf.FAT_buffer[tmpr%FEPS] = temp;
        writeSector(fp,FAT,tmpr/FEPS);
       
        readSector(fp,mbr,0);
        buf.mbr_buffer.start_of_first_fb = temp1;
        writeSector(fp,mbr,0);
        
        readSector(fp,data,temp);
        printf("\nEnter the data: ");
        printf("\nPress Esc Key for end-of-file\n\n");
        for(i=0;i<SIZE_OF_SECTOR;i++)
         {
           buf.data_buffer[i] = getch();
           if(buf.data_buffer[i] == 27)
            {
              buf.data_buffer[i] = '\0';
              flag = 1;
              break;
             }
           if(buf.data_buffer[i] == 13)
              putchar('\n');
           else
              putchar(buf.data_buffer[i]);
           }     
        fflush(stdin);
        size = size + strlen(buf.data_buffer);
        size_on_disk++; 
        writeSector(fp,data,temp);
    }
    
    
    while(!flag)   /*more data is to be appended*/
    {
        tmpr = temp;
        readSector(fp,mbr,0);
        temp= buf.mbr_buffer.start_of_first_fb;
        
        /*check if disk is full*/
        if(temp == -1)
         {
           printf("\n\nDISK IS FULL\n");
           return;
         }
        
        readSector(fp,FAT,temp/256);
        tempfb = buf.FAT_buffer[temp%256];
        
        readSector(fp,mbr,0);
        buf.mbr_buffer.start_of_first_fb = tempfb;
        writeSector(fp,mbr,0);
        
        readSector(fp,FAT,tmpr/256);
        buf.FAT_buffer[tmpr%256] = temp;
        writeSector(fp,FAT,tmpr/256);
        
        readSector(fp,FAT,temp/256);
        buf.FAT_buffer[temp%256] = -1;
        writeSector(fp,FAT,temp/256);
        
        for(i=0;i<SIZE_OF_SECTOR;i++)
         {
           buf.data_buffer[i] = getch();
           if(buf.data_buffer[i] == 27)
            {
              buf.data_buffer[i] = '\0';
              flag = 1;
              break;
             }
           if(buf.data_buffer[i] == 13)
              putchar('\n');
           else
              putchar(buf.data_buffer[i]);
          }     
        
       size = size + strlen(buf.data_buffer);
       size_on_disk++;
       writeSector(fp,data,temp);
     }  
    
    update(fp,curdir,size_on_disk,size,1);
    readSector(fp,dirStruct,curdir); 
    buf.dirStruct_buffer.dir_array[index].size += size;
    buf.dirStruct_buffer.dir_array[index].size_on_disk += size_on_disk;
    buf.dirStruct_buffer.dir_array[index].date_modified = time(0);
    writeSector(fp,dirStruct,curdir);
    printf("\n\n\nFile has been appended successfully!\n");
   
}
/****************************************************************************/

/* to overwrite already existing contents of a file */
void overwrite(char *name, int curdir, FILE *fp)
{
     int index,temp,tmpr,flag,size,sizeOnDisk,tempfb;
     int i,j;
     char ans;
     
     flag = 0;
     if (strstr(name,".txt")==NULL)
     {
         printf("Cannot overwrite directory\n");
         return;
     }
     
     /*read Sector corresponding to current directory*/
     readSector(fp,dirStruct,curdir);
     checkName(name,&curdir,&index,fp);  
    
     if((buf.dirStruct_buffer.dir_array[index].dir_attributes & 1)==1)
       {
          printf("\nFile is read-only!!");
          printf("\nYou cannot overwrite......");
          return;
        }
        
     /*to overwrite specified file,
       first free already allocated sectors*/
     temp = buf.dirStruct_buffer.dir_array[index].starting_address;
     
     /* updating mbr */
     readSector(fp,mbr,0);
     tmpr = buf.mbr_buffer.start_of_first_fb;
     buf.mbr_buffer.start_of_first_fb = temp;
     writeSector(fp,mbr,0);
     
     /* updating FAT */
     readSector(fp,FAT,temp/FEPS);
     while (buf.FAT_buffer[temp%FEPS]!=-1)
     {
           temp = buf.FAT_buffer[temp%FEPS];
           readSector(fp,FAT,temp/FEPS);
     }
     buf.FAT_buffer[temp%FEPS] = tmpr;
     writeSector(fp,FAT,temp/FEPS);
     
     /*overwrite now.......
       writing new data at the same address
     */

     readSector(fp,dirStruct,curdir);
     size = buf.dirStruct_buffer.dir_array[index].size;
     buf.dirStruct_buffer.dir_array[index].size = 0;
     sizeOnDisk = buf.dirStruct_buffer.dir_array[index].size_on_disk ;
     buf.dirStruct_buffer.dir_array[index].size_on_disk = 0;
     temp = buf.dirStruct_buffer.dir_array[index].starting_address;
     writeSector(fp,dirStruct,curdir);
     update(fp,curdir,-sizeOnDisk,-size,1);
     size = 0;
     sizeOnDisk = 0;
     
     readSector(fp,FAT,temp/FEPS);
     tmpr = buf.FAT_buffer[temp%FEPS];
     buf.FAT_buffer[temp%FEPS] = -1;
     writeSector(fp,FAT,temp/FEPS);
           
     readSector(fp,mbr,0);
     buf.mbr_buffer.start_of_first_fb = tmpr;
     writeSector(fp,mbr,0);
           
     readSector(fp,data,temp);
     printf("\nEnter the data:");
     printf("\nPress Esc Key for end-of-file\n\n");
     for(i=SIZE_OF_PASSWORD;i<SIZE_OF_SECTOR;i++)
         {
           buf.data_buffer[i] = getch();
           if(buf.data_buffer[i] == 27)
            {
              buf.data_buffer[i] = '\0';
              flag = 1;
              break;
             }
           if(buf.data_buffer[i] == 13)
              putchar('\n');
           else
              putchar(buf.data_buffer[i]);
           }     
     fflush(stdin);
     size = strlen(buf.data_buffer);
     size = size - SIZE_OF_PASSWORD;
     sizeOnDisk++;
           
     buf.data_buffer[size+SIZE_OF_PASSWORD] = '\0';
     writeSector(fp,data,temp);
     
     
     
     
      while(!flag)   /*more data is there....*/
       {
        tmpr = temp;
        readSector(fp,mbr,0);
        temp= buf.mbr_buffer.start_of_first_fb;
        /*check if disk is full*/
        if(temp == -1)
         {
           printf("\n\nDISK IS FULL\n");
           return;
         }
        
        readSector(fp,FAT,temp/256);
        tempfb = buf.FAT_buffer[temp%256];
        
        readSector(fp,mbr,0);
        buf.mbr_buffer.start_of_first_fb = tempfb;
        writeSector(fp,mbr,0);
        
        readSector(fp,FAT,tmpr/256);
        buf.FAT_buffer[tmpr%256] = temp;
        writeSector(fp,FAT,tmpr/256);
        
        readSector(fp,FAT,temp/256);
        buf.FAT_buffer[temp%256] = -1;
        writeSector(fp,FAT,temp/256);
        
        for(i=0;i<SIZE_OF_SECTOR;i++)
         {
           buf.data_buffer[i] = getch();
           if(buf.data_buffer[i] == 27)
            {
              buf.data_buffer[i] = '\0';
              flag = 1;
              break;
             }
           if(buf.data_buffer[i] == 13)
              putchar('\n');
           else
              putchar(buf.data_buffer[i]);
          }     
        
       size = size + strlen(buf.data_buffer);
       sizeOnDisk++;
       writeSector(fp,data,temp);
     }  
    
     update(fp,curdir,sizeOnDisk,size,1);
     
     /* updating directory entry of the file */
     readSector(fp,dirStruct,curdir);
     buf.dirStruct_buffer.dir_array[index].date_modified = time(0);
     buf.dirStruct_buffer.dir_array[index].size = size;
     buf.dirStruct_buffer.dir_array[index].size_on_disk = sizeOnDisk;
     writeSector(fp,dirStruct,curdir);
     
     printf("\n\n\nFile has been overwritten successfully!\n");
}
/***************************************************************************/


void setPass(char *name,int curdir,FILE *fp)
{
     int i,check,index;
     int startAdd;
     char a;
     
     if(*name == '\0')
     {
       printf("\nNo file name specified");
       return;
     }
     if(strstr(name,".txt")==NULL)
     {
         printf("\nSpecified argument must be a file, not a directory");
         return;
     }
       
     readSector(fp,dirStruct,curdir);
     check = checkName(name,&curdir,&index,fp);
     //there has been no collision
     if(check!=0)
     {
             printf("This file does not exist!\n");
             return;
     }
       
     readSector(fp,dirStruct,curdir);
     startAdd = buf.dirStruct_buffer.dir_array[index].starting_address;
     //password bit is set
     if((buf.dirStruct_buffer.dir_array[index].dir_attributes & 32) == 32)
     {
            printf("\n Password is already set!!");
            printf("\n To edit the existing password, use editPasswrd: ");
     }
     else //password bit is not set
     {
          //set password bit
          buf.dirStruct_buffer.dir_array[index].dir_attributes = 
                     buf.dirStruct_buffer.dir_array[index].dir_attributes | 32;
          writeSector(fp,dirStruct,curdir);
          //password will be stored in first 6 bytes of file
          readSector(fp,data,startAdd);
          printf("\nPassword must be of 6 characters");
          printf("\nEnter the password: ");
          for (i=0;i<SIZE_OF_PASSWORD;i++)
          {
              a = getch();
              buf.data_buffer[i] = a;
              putchar('*');
          }
          sleep(500);
          fflush(stdin);
                
          printf("\nConfirm password: ");
          for (i=0;i<SIZE_OF_PASSWORD;i++)
          {
              a = getch();
              putchar('*');
              if(a!=buf.data_buffer[i])
              {
                    printf("\nPassword does not match,cannot set password!\n");
                    readSector(fp,dirStruct,curdir);
                    //reset password bit, user could not confirm password
                    buf.dirStruct_buffer.dir_array[index].dir_attributes = 
                    buf.dirStruct_buffer.dir_array[index].dir_attributes & 31;
                    writeSector(fp,dirStruct,curdir);
                    return;
              } 
          }
          sleep(500);
          //valid password has been entered by the user
          writeSector(fp,data,startAdd);    
          printf("\nPassword has been set successfully");
     } 
}
/****************************************************************************/

void editPass(char name[], int curdir, FILE *fp)
{
     int startAdd,i,index,check;
     char a;
     readSector(fp,dirStruct,curdir);
     check = checkName(name,&curdir,&index,fp);
     if(*name == '\0')
      {
       printf("\nNo file name specified");
       return;
      }
     if (check)
     {
               printf("No such file exists!\n");
               return;
     }
     readSector(fp,dirStruct,curdir);
     if ((buf.dirStruct_buffer.dir_array[index].dir_attributes& 32) != 32)
     {
         printf("Password is not set! use setP to set password\n");
         return;
     }
     //password bit is set
     printf("\nTo set new password, you have to enter old password first....");
     check = accessFile(name,curdir,&startAdd,fp);
     //user has entered correct password
     if (check ==1)
     {
              printf("\nEnter new password: ");
              for (i=0;i<SIZE_OF_PASSWORD;i++)
              {
                  a = getch();
                  putchar('*');
                  buf.data_buffer[i] = a;
              }
              sleep(500);
              printf("\nConfirm password: ");
              for (i=0;i<SIZE_OF_PASSWORD;i++)
              {
                  a = getch();
                  putchar('*');
                  if(a!=buf.data_buffer[i])
                  {
                     /*if user is unable to confirm password, 
                                      old password will remain*/
                     printf("\nPassword does not match!!");
                     return;
                  }
              }
              sleep(500);
              writeSector(fp,data,startAdd);
              printf("\nPassword changed successfully!\n");
     }
     else
         printf("\nSorry, incorrect password!\n");
}
/****************************************************************************/

void removePass(char name[],int curdir,FILE *fp)
{
     int check,index,dumm,i;
     char a;
     
     readSector(fp,dirStruct,curdir);
     check = checkName(name,&curdir,&index,fp);
     readSector(fp,dirStruct,curdir);
     if(*name == '\0')
      {
       printf("\nNo file name specified");
       return;
      }
     if ((buf.dirStruct_buffer.dir_array[index].dir_attributes& 32) != 32)
     {
         printf("There is no password set on this file!\n");
         return;
     }
     
     check = accessFile(name,curdir,&dumm,fp);
     //we can access file, user has entered correct password
     if (check ==1)
     {
              readSector(fp,dirStruct,curdir);
              //reset password bit
              buf.dirStruct_buffer.dir_array[index].dir_attributes = 
                 buf.dirStruct_buffer.dir_array[index].dir_attributes & 31;
              writeSector(fp,dirStruct,curdir);
              printf("\nPassword removed successfully!\n");
     }
     else //user has not entered correct password
         printf("\nSorry, incorrect password!\n");
}
/****************************************************************************/









/****************************************************************************/

void display_RBsize(FILE *fp)
{
     int i,temp;
     int size = 0;
     readSector(fp,mbr,0);
     temp = buf.mbr_buffer.start_of_recycle_bin;
     
     while(1)
     {
          readSector(fp,dirStruct,temp);
          for(i=0;i<DEPS;i=i+2)
          {
            //valid entry
           if((buf.dirStruct_buffer.dir_array[i].dir_attributes & 16) == 16)
             size = size+(buf.dirStruct_buffer.dir_array[i].size_on_disk*1024);
          }
          
          readSector(fp,FAT,temp/FEPS);
          if(buf.FAT_buffer[temp%FEPS]==-1)
            break;
          temp = buf.FAT_buffer[temp%FEPS];      
     }
       
     printf("\nUsed Recycle-bin space = %f",
                              (((float)size/(float)SIZE_OF_RECYCLE_BIN)*100));
     printf(" %%");
     printf("\n\nFree Recycle-bin space = %f",
                          100-(((float)size/(float)SIZE_OF_RECYCLE_BIN)*100));
     printf(" %%");
     printf("\n\n");
}

/****************************************************************************/

void addStrings(char path[512], char object[])
{
     int length = strlen(object);
     int pathlength = strlen(path);
     int i,j;
     for (i=pathlength,j=0;i<length+pathlength;i++,j++)
     {   
        path[i] = object[j];
     }
     path[i] = '\0';
}

/****************************************************************************/
void sendToBin(int *size_of_bin, char* name, int curdir,
                                          struct dir_entry* mainTemp, FILE *fp)
{
     
     int index,check,flag,temp,tempfb,half,i,j,temprb;
     struct dir_entry* tmpr=(struct dir_entry*)malloc(sizeof(struct dir_entry));
     char path[512];
     
     //this string will store path from root to file/folder to be deleted
     path[0] = '\0';
     
     readSector(fp,dirStruct,curdir);
     check = checkName(name,&curdir,&index,fp);
     readSector(fp,dirStruct,curdir);
     if (check)
     {
              printf("No such File exists!\n");
              return;
     }
     
     flag = 0;
     //flag is 1 when file/folder is too large to be sent to the recycle bin
     
     if((*size_of_bin + buf.dirStruct_buffer.dir_array[index].size)
                                                        > SIZE_OF_RECYCLE_BIN)
     {
          printf("\nSorry !! Recycle bin is already full");
          flag = 1;
     }
        
     if(!flag)
     {
         if(buf.dirStruct_buffer.dir_array[index].size > 5*SIZE_OF_SECTOR)
         {
            printf("This file is too large to be sent to recycle bin\n");
            flag = 1;
         }
     }
      
     if(!flag)
       *size_of_bin = *size_of_bin + buf.dirStruct_buffer.dir_array[index].size; 
     else
     {
          while (1)
          {
                printf("Enter your option:\n");
                printf("1. Delete the file from disk\n");
                printf("2. Cancel deletion\n");
                scanf("%d",&check);
                if (check==1)
                   del_f(name,curdir,tmpr,fp);
                else if (check==2)
                   printf("Deletion Operation Aborted!\n");
                else continue;
                //keep asking user till he enters valid input
                return;
          }
     }
     /*if file we are sending to bin, has been cut or copied previously,
                                                  then it must be invalidated*/
     if (buf.dirStruct_buffer.dir_array[index].starting_address
                                                 == mainTemp->starting_address)
        mainTemp->starting_address = -1;
        
     readSector(fp,dirStruct,curdir);
     cut(tmpr,name,curdir,fp);
     readSector(fp,mbr,0);
     temprb = buf.mbr_buffer.start_of_recycle_bin;
     tempfb = buf.mbr_buffer.start_of_first_fb;
     
     /*check if disk is full*/
     readSector(fp,FAT,tempfb/256);
     if(buf.FAT_buffer[tempfb%256] == -1)
      {
       printf("\n\nDISK IS FULL\n");
       return;
      }
       
     paste(tmpr,&temprb,&index,fp);
     index++;
     //setting the parent directory entry
     readSector(fp,dirStruct,temprb);
     buf.dirStruct_buffer.dir_array[index].starting_address = -1;
     buf.dirStruct_buffer.dir_array[index].dir_attributes = 
        buf.dirStruct_buffer.dir_array[index].dir_attributes | 16;
     writeSector(fp,dirStruct,temprb);
     index--;
     if (index%4 == 0)
     {
              half = 1;
              //first half of data sector will be used to store path
              if (buf.dirStruct_buffer.dir_array[index+3].starting_address==-1)
              {
                   readSector(fp,mbr,0);
                   tempfb = buf.mbr_buffer.start_of_first_fb;
                   readSector(fp,FAT,tempfb/FEPS);
                   temp = buf.FAT_buffer[tempfb%FEPS];
                   buf.FAT_buffer[tempfb%FEPS] = -1;
                   writeSector(fp,FAT,tempfb/FEPS);
                   
                   readSector(fp,mbr,0);
                   buf.mbr_buffer.start_of_first_fb = temp;
                   writeSector(fp,mbr,0);
                   //allocate new block
              }
              else
               tempfb=buf.dirStruct_buffer.dir_array[index+3].starting_address;
     }
     else
     {
             //second half will be used to store path
             half = 2;
             if (buf.dirStruct_buffer.dir_array[index-1].starting_address==-1)
             {
                   //allocate new block
                   readSector(fp,mbr,0);
                   tempfb = buf.mbr_buffer.start_of_first_fb;
                   readSector(fp,FAT,tempfb/FEPS);
                   temp = buf.FAT_buffer[tempfb%FEPS];
                   buf.FAT_buffer[tempfb%FEPS] = -1;
                   writeSector(fp,FAT,tempfb/FEPS);
                   
                   readSector(fp,mbr,0);
                   buf.mbr_buffer.start_of_first_fb = temp;
                   writeSector(fp,mbr,0);
             }
             else
                tempfb=buf.dirStruct_buffer.dir_array[index-1].starting_address;
     }
     readSector(fp,dirStruct,temprb);
     buf.dirStruct_buffer.dir_array[index+1].starting_address = tempfb;
     writeSector(fp,dirStruct,temprb);
     
     addStrings(path,"\nroot");
        
     for(i=0; i<tos; i++)
     {
       flag = 0;
       temp = family_stack[i];
       readSector(fp,dirStruct,temp);
       while (1)
       {
          for (j=0;j<DEPS;j++)
          {
              if((buf.dirStruct_buffer.dir_array[j].starting_address)
                                                         == family_stack[i+1] )
              {
                                                      
                  addStrings(path,"\\");
                  addStrings(path,buf.dirStruct_buffer.dir_array[j].name);                
                  flag = 1;
                  break;
              }
          }
          if(flag==1)
            break;
          //FAT[temp] != -1
          j = temp/FEPS;
          readSector(fp,FAT,j);
          temp = buf.FAT_buffer[temp%FEPS];
          
          readSector(fp,dirStruct,temp);
       }
     }//path now contains path from root to file/folder
     readSector(fp,data,tempfb);
     //storing this path in first or second half, depending on value of half
     if(half ==1)
     {
       for(i=0,j=0;i<512;i++,j++)
          buf.data_buffer[i] = path[j];
     }
     else
     {
          for(i=512,j=0;i<SIZE_OF_SECTOR;i++,j++)
             buf.data_buffer[i] = path[j];         
     }
     writeSector(fp,data,tempfb);
}
/****************************************************************************/

void del_RB(char* name, FILE *fp)
{
     int temprb,index,check,tempfb,startdir;
     int flag = 0;
     struct dir_entry* dum=(struct dir_entry*)malloc(sizeof(struct dir_entry));
     
     readSector(fp,mbr,0);
     temprb = buf.mbr_buffer.start_of_recycle_bin;
     
     startdir = temprb;
     
     readSector(fp,dirStruct,temprb);
     check = checkName(name,&temprb,&index,fp);
     
     if (!check)
     {
        del_f(name,temprb,dum,fp);
        readSector(fp,dirStruct,temprb);
        //resetting parent entry
        buf.dirStruct_buffer.dir_array[index+1].dir_attributes = 
           buf.dirStruct_buffer.dir_array[index+1].dir_attributes & 0;
        check = buf.dirStruct_buffer.dir_array[index+1].starting_address;
        buf.dirStruct_buffer.dir_array[index+1].starting_address = -1;
        writeSector(fp,dirStruct,temprb);
        //flag = 1 indicates data sector must be freed
        remove_struct(startdir,temprb,fp);
        if((index%4)==0)
        {
          if(buf.dirStruct_buffer.dir_array[index+3].starting_address == -1)
             flag = 1;
        }
        else
        {
            if(buf.dirStruct_buffer.dir_array[index-1].starting_address == -1)
               flag = 1;
        }
        if(flag)  /* if we need to free data sector */
        {
             /* updating mbr */
             readSector(fp,mbr,0);
             tempfb = buf.mbr_buffer.start_of_first_fb;
             buf.mbr_buffer.start_of_first_fb = check;
             writeSector(fp,mbr,0);
             
             /* updating fat */
             readSector(fp,FAT,check/FEPS);
             buf.FAT_buffer[check%FEPS] = tempfb;
             writeSector(fp,FAT,check/FEPS);
        }
     }
     else
        printf("No such file\\folder exists!!\n");
}
/****************************************************************************/

void emptyRB(FILE *fp)
{
     int temprb,i;
     readSector(fp,mbr,0);
     temprb = buf.mbr_buffer.start_of_recycle_bin;
     
     do
     {
         readSector(fp,dirStruct,temprb);
         //call delete for every valid entry in recycle bin
         for (i=0;i<DEPS;i=i+2)
             if ((buf.dirStruct_buffer.dir_array[i].dir_attributes & 16 )== 16)
             {
                del_RB(buf.dirStruct_buffer.dir_array[i].name,fp);
                readSector(fp,dirStruct,temprb);
             }
         readSector(fp,FAT,temprb/FEPS);
         temprb = buf.FAT_buffer[temprb%FEPS];
     }while (temprb!=-1);
}
/****************************************************************************/
void move_to_disk(int curdir, int fl,int addr,struct dir_entry* tmpr, FILE *fp)
{
     int start,i,check,tcurdir;
     char* temp;
     char name[11];
     char ans;
     char path[SIZE_OF_SECTOR/2];
     struct dir_entry* tmp=(struct dir_entry*)malloc(sizeof(struct dir_entry));
     readSector(fp,dirStruct,curdir);
     strcpy(name,buf.dirStruct_buffer.dir_array[fl].name);
     readSector(fp,data,addr);
     
     if (fl%4!=0)
         start = SIZE_OF_SECTOR/2;
     else
         start = 0;
         
     for (i=0;i<SIZE_OF_SECTOR/2;i++)
         path[i] = buf.data_buffer[start + i];
         
     temp = strtok(path,"\\");
     tos = -1;
     readSector(fp,mbr,0);
     push(buf.mbr_buffer.start_of_root);
     curdir = buf.mbr_buffer.start_of_root;
     
     temp = strtok(NULL,"\\");
     while (temp!=NULL)
     {
           readSector(fp,dirStruct,curdir);
           check = checkName(temp,&curdir,&fl,fp);
           readSector(fp,dirStruct,curdir);
           if (check==0)
           {
             push(buf.dirStruct_buffer.dir_array[fl].starting_address);
             curdir = buf.dirStruct_buffer.dir_array[fl].starting_address;
           }
           else
               curdir = createDir(curdir,temp,fp);
           temp = strtok(NULL,"\\");         
     }
     readSector(fp,dirStruct,curdir);
     tcurdir = curdir;
     check = checkName(name,&tcurdir,&fl,fp);
     if (check==0)
     {
                 printf("\n\nThe destination folder already contains the ");
                 printf(" given file/folder\n");
                 printf("Would you like to replace the existing file with ");
                 printf("this file?\n\nType 'y' for yes\n");
                 scanf("%c",&ans);
                 if (ans!='y'&&ans!='Y')
                    return;
                 else
                    del_f(name,curdir,tmpr,fp);
     }               
     readSector(fp,mbr,0);
     cut(tmp,name,buf.mbr_buffer.start_of_recycle_bin,fp);
     paste(tmp,&curdir,&fl,fp);
}
/*****************************************************************************/
void restore_from_bin(char name[], struct dir_entry *tmp, FILE *fp)
{
     int check, index,temprb,tempfb,startdir;
     int startAdd;
     char ans;
     
     readSector(fp,mbr,0);
     temprb = buf.mbr_buffer.start_of_recycle_bin;
     tempfb = buf.mbr_buffer.start_of_first_fb;
     
     startdir = temprb;
     
     readSector(fp,dirStruct,temprb);
     //reading first sector of recycle bin
     
     
     check = checkName(name,&temprb,&index,fp);
     //temp now contains, the current dir structure
     
     if(check)
     {
          printf("\nThis file\\folder is not present in recycle bin");
          return;
     }
     
     /* check = 0 */
     /* invalidating entries..... */
                    
     /* entry corresponding to its parent */
     buf.dirStruct_buffer.dir_array[index+1].dir_attributes = 
                    buf.dirStruct_buffer.dir_array[index+1].dir_attributes & 0;
     startAdd = buf.dirStruct_buffer.dir_array[index+1].starting_address;
     buf.dirStruct_buffer.dir_array[index+1].starting_address = -1;
     writeSector(fp,dirStruct,temprb);
     
     remove_struct(startdir,temprb,fp);
     if (index%4==0)
     {
               if(buf.dirStruct_buffer.dir_array[index+3].starting_address==-1)
                 {
                     readSector(fp,mbr,0);
                     buf.mbr_buffer.start_of_first_fb = startAdd;
                     writeSector(fp,mbr,0);
                     readSector(fp,FAT,startAdd/FEPS);
                     buf.FAT_buffer[startAdd%FEPS] = tempfb;
                     writeSector(fp,FAT,startAdd/FEPS);
                 }   //free buf.dirStruct_buffer.dir_array[fl].starting_address
     }
     else
     {
               if(buf.dirStruct_buffer.dir_array[index-1].starting_address==-1)
                 {
                     readSector(fp,mbr,0);
                     buf.mbr_buffer.start_of_first_fb = startAdd;
                     writeSector(fp,mbr,0);
                     readSector(fp,FAT,startAdd/FEPS);
                     buf.FAT_buffer[startAdd%FEPS] = tempfb;
                     writeSector(fp,FAT,startAdd/FEPS);
                 }   //free buf.dirStruct_buffer.dir_array[fl].starting_address
     } 
     move_to_disk(temprb,index,startAdd,tmp,fp);
}
/****************************************************************************/

void restore_bin(struct dir_entry *tmp, FILE *fp)
{
     int temprb;
     int i;
     
     readSector(fp,mbr,0);
     temprb = buf.mbr_buffer.start_of_recycle_bin;
     
     while(1)
     {
            readSector(fp,dirStruct,temprb);
            for(i=0;i<DEPS;i=i+2)
            {
                /* restore all valid enteries */
              if((buf.dirStruct_buffer.dir_array[i].dir_attributes & 16) == 16)
              {
               restore_from_bin(buf.dirStruct_buffer.dir_array[i].name,tmp,fp);
               readSector(fp,dirStruct,temprb);
              }    
            }

            readSector(fp,FAT,temprb/FEPS);
            if(buf.FAT_buffer[temprb%FEPS] == -1)
                break;
            temprb = buf.FAT_buffer[temprb%FEPS];
      }    
}
/****************************************************************************/

void displayRB(int dir,FILE *fp)
{
     char ans;
     int temp,i;
     char flag;
     int set;
     
     flag = 'f';
     
     while (1)
     {
       readSector(fp,dirStruct,dir);
       for(i=0;i<DEPS;i=i+2)
       {
           if((buf.dirStruct_buffer.dir_array[i].dir_attributes&16)==16)
           {
              set = 0;
              printf("\n\n");
              flag = 't';
               
              printf("\n\n NAME          : %s",
                                       buf.dirStruct_buffer.dir_array[i].name);
              if((buf.dirStruct_buffer.dir_array[i].dir_attributes&8)==8)
                printf("\n TYPE          : FOLDER  ");
              else
                printf("\n TYPE          : TEXT FILE  ");
                    
              printf("\n DATE CREATED  : %s",
                       ctime(&buf.dirStruct_buffer.dir_array[i].date_created));
              printf(" DATE MODIFIED : %s",
                      ctime(&buf.dirStruct_buffer.dir_array[i].date_modified));
              printf(" SIZE ON DISK  : %d KB",
                               buf.dirStruct_buffer.dir_array[i].size_on_disk);
              printf("\n SIZE          : %d Bytes",
                                       buf.dirStruct_buffer.dir_array[i].size);
              printf("\n ATTRIBUTES    :");
                    
              if((buf.dirStruct_buffer.dir_array[i].dir_attributes&4)==4)
               {
                printf(" HIDDEN  ");
                set = 1;
               }
              if((buf.dirStruct_buffer.dir_array[i].dir_attributes&2)==2)
               {
                printf(" ARCHIVE  ");
                set = 1;
               }
              if((buf.dirStruct_buffer.dir_array[i].dir_attributes&1)==1)
              {
                printf(" READ-ONLY  ");
                set = 1;
              }
              if((buf.dirStruct_buffer.dir_array[i].dir_attributes&32)==32)
              {
                printf(" PASSWORD PROTECTED  \n");
                set = 1;
              }
              if(!set)
               printf(" NO ATTRIBUTE IS SET");
           }
       }
       readSector(fp,FAT,dir/FEPS);   
       if(buf.FAT_buffer[dir%FEPS]!=-1)
            dir = buf.FAT_buffer[dir%FEPS];
       else
            break;
     }
     
     if(flag=='f')
        printf("\nNo files\\folders found..\n");
     printf("\n\n");
}
/****************************************************************************/
void RB_manual(char *temp,int choice)
{
  if(choice==0)
    {
        printf("\n\n             LIST OF COMMANDS:");
        printf("\n---------------------------------------------\n");
        printf("\n del        delete file\\folder");
        printf("\n empty      empty recycle bin");
        printf("\n restore    restore file\\folder");
        printf("\n restoreAll restore all files and folders");
        printf("\n ls         list contents");
        printf("\n size       space: used and free\n");
        printf("\n---------------------------------------------\n");
        printf("\nType HELP command-name to get detailed information ");
        printf("of a particular command\n\n");
      }
  else
   {
      if(strcmpi(temp,"ls")==0)
       {
         printf("\n - ls");
         printf("\n");
         printf("\n   lists the contents of recycle bin");
         printf("\n   along with their attributes");
         printf("\n\n");
       }
      else if(strcmpi(temp,"del")==0)
       {
         printf("\n - del File/Folder Name");
         printf("\n");
         printf("\n   deletes specified file/folder from recycle bin");
         printf("\n   Example - del temp");
         printf("\n   deletes temp folder from  recycle bin");
         printf("\n\n");
       }
      else if(strcmpi(temp,"empty")==0)
       {
           printf("\n - empty");
           printf("\n");
           printf("\n   deletes all files and folders from recycle bin");
           printf("\n   Example - empty");
           printf("\n   empties recycle bin");
           printf("\n\n");
       }
      else if(strcmpi(temp,"restore")==0)
       {
           printf("\n - restore File/Folder Name");
           printf("\n");
           printf("\n   restores specified file/folder from recycle bin");
           printf("\n   to its original location");
           printf("\n   Example - restore abc.txt");
           printf("\n   restores abc.txt file from  recycle bin");
           printf("\n\n");
       }
      else if(strcmpi(temp,"restoreAll")==0)
       {
           printf("\n - restoreAll");
           printf("\n");
           printf("\n   restores all files and folders from recycle bin");
           printf("\n   to their respective original locations");
           printf("\n   Example - restoreAll");
           printf("\n   empties recycle bin and restores all contents");
           printf("\n\n");
       }
      else if(strcmpi(temp,"size")==0)
       {
           printf("\n - size");
           printf("\n");
           printf("\n   prints used and free disk space in the recycle bin");
           printf("\n\n");
       }
   }
}
/****************************************************************************/
void recycler(struct dir_entry *tmpr, FILE *fp)
{
     char input[50];
     char *temp;
     int func = 0;
     int t;
     
     readSector(fp,mbr,0);
     t = buf.mbr_buffer.start_of_recycle_bin;
     
     printf("\n\nType HELP to get a list of commands");
     printf("\nType EXIT to exit from recycle bin\n\n");
     
     while(1)
     { 
             printf("\nRecycle-Bin>");
             fflush(stdin);
             gets(input);
             fflush(stdin);
      
             if(strcmpi(input,"ls")==0)
                func = 1;
             else if(startsWith(input,"del "))
                func = 2;
             else if(strcmpi(input,"empty")==0)
                func = 3;
             else if(startsWith(input,"restore "))
                func = 4;
             else if(strcmpi(input,"restoreAll")==0)
                func = 5;
             else if(strcmpi(input,"size")==0)
                func = 6;
             else if((strcmpi(input,"help")==0)||(startsWith(input,"help "))
                                                  ||(startsWith(input,"HELP ")))
                func = 7;
             else if(strcmpi(input,"exit")==0)
                func = 10;
             else 
                func = 0;
             
             switch(func)
             {  
                  case 0: printf("Bad command or file name\n");
                          break;
                  case 1: displayRB(t,fp);
                          break;
                  case 2: temp = &input[4];
                          del_RB(temp,fp);
                          break;
                  case 3: emptyRB(fp);
                          break;
                  case 4: temp = &input[8];
                          restore_from_bin(temp,tmpr,fp);
                          break;
                  case 5: restore_bin(tmpr,fp);
                          break;
                  case 6: display_RBsize(fp);
                          break;
                  case 7: temp = &input[4];
                          if((*temp=='\t')||(*temp=='\0'))
                            RB_manual(temp,0);
                          else
                            {
                               temp = &input[5];
                               RB_manual(temp,1);
                            }
                          break;
                  case 10:return;
             }     
     }    
}
/****************************************************************************/

void display_manual(char *temp,int choice)
{
    if(choice == 0)
     {
        printf("\n\n       LIST OF COMMANDS:");
        printf("\n-------------------------------\n");
        printf("\n append     append data");
        printf("\n bin:       open recycle bin");
        printf("\n cd         change directory");
        printf("\n cd..       move to parent");
        printf("\n cd\\        move to root");
        printf("\n chmode     modify attributes");
        printf("\n copy       copy file\\folder");
        printf("\n cut        cut file\\folder");
        printf("\n del        send to bin");
        printf("\n editP      edit password");
        printf("\n format     format disk");
        printf("\n ls         list contents");
        printf("\n md         create directory");
        printf("\n mk         create file");
        printf("\n open       open file");
        printf("\n overwrite  overwrite data");
        printf("\n paste      paste file\\folder");
        printf("\n remP       remove password");
        printf("\n rm         rename file\\folder");
        printf("\n sdel       delete from disk");
        printf("\n setP       set password");
        printf("\n size       space: free and used\n");
        printf("\n---------------------------------\n");
        printf("\nType HELP command-name to get detailed information of ");
        printf("a particular command\n\n");
      }
    else
     {
        if(strcmpi(temp,"md")==0)  
         {
             printf("\n - md Directory Name");
             printf("\n");
             printf("\n   Creates directory with the directory name ");
             printf("\n   specified as an argument with no attributes set");
             printf("\n   Creates directory with name 'New Folder' ");
             printf("if nothing is specified");
             printf("\n   Example - md temp");
             printf("\n   Creates a directory named temp in current directory");
             printf("\n\n");
         }
        else if(strcmpi(temp,"mk")==0)
         {
             printf("\n - mk File Name");
             printf("\n");
             printf("\n   Creates file with the file name specified ");
             printf("as an argument ");
             printf("\n   with no attributes set");
             printf("\n   and creates file 'noname.txt' if nothing is ");
             printf("specified as an argument ");
             printf("\n   Example - mk abc");
             printf("\n   Creates a file named abc.txt in current directory");
             printf("\n\n");
         }
        else if(strcmpi(temp,"sdel")==0)
          {
             printf("\n - sdel file\\folder Name");
             printf("\n");
             printf("\n   Deletes specified file or folder if it exists ");
             printf("in current directory");
             printf("\n   Example - del temp");
             printf("\n   Deletes directory named temp from current directory");
             printf("\n   and all the files and folders contained in temp");
             printf("\n\n");
          }
        else if(strcmpi(temp,"rm")==0)
          {
             printf("\n - rm src_file\\folder dest_file\\folder");
             printf("\n");
             printf("\n   Renames src_file\\folder with the name specified ");
             printf("as dest_file\\folder");
             printf("\n   Example - rm abc.txt def.txt");
             printf("\n   Renames abc.txt as cde.txt");
             printf("\n\n");
          }
         else if(strcmpi(temp,"ls")==0)
           {
              printf("\n - ls");
              printf("\n");
              printf("\n   List contents of current directory");
              printf("\n   Displays all files and folders contained ");
              printf("in current directory");
              printf("\n\n");
           }
          else if(strcmpi(temp,"open")==0)
           {
               printf("\n - open File Name");
               printf("\n");
               printf("\n  Open a file whose name is specified as an argument");
               printf("\n  Example - open abc.txt");
               printf("\n  Opens file abc.txt if it exists in current ");
               printf("directory");
               printf("\n\n");
           }
          else if(strcmpi(temp,"append")==0)
            {
               printf("\n - append File Name");
               printf("\n");
               printf("\n   Appends data entered by user, in the file ");
               printf("specified as an argument");
               printf("\n   Example - append abc.txt");
               printf("\n   Appends data entered by user in file abc.txt");
               printf("\n\n");
            }
          else if(strcmpi(temp,"overwrite")==0)
            {
               printf("\n - overwrite File Name");
               printf("\n");
               printf("\n   Overwrites existing contents of the file ");
               printf("specified as an argument,");
               printf("\n   by the data entered by user");
               printf("\n   Example - overwrite abc.txt");
               printf("\n   Overwrites contents of abc.txt by the data ");
               printf("entered by user");
               printf("\n\n");
            }
          else if(strcmpi(temp,"cut")==0)
            {
               printf("\n - cut File/Folder Name");
               printf("\n");
               printf("\n   cuts the file/folder specified as an argument,");
               printf("from the current directory");
               printf("\n   and saves it in some temporary location");
               printf("\n   Example - cut abc.txt");
               printf("\n   cuts abc.txt from the current directory");
               printf("\n\n");
            }
           else if(strcmpi(temp,"copy")==0)
             {
                printf("\n - copy File\\Folder Name");
                printf("\n");
                printf("\n   Copies file\\folder specified as an argument,");
                printf("\n   Example - copy abc.txt");
                printf("\n   copy abc.txt");
                printf("\n\n");
             }
            else if(strcmpi(temp,"paste")==0)
             {
                 printf("\n - paste");
                 printf("\n");
                 printf("\n   pastes  the contents of temporary location in ");
                 printf("current directory");
                 printf("\n\n");
             }
            else if(strcmpi(temp,"chmode")==0)
             {
                 printf("\n - chmode file\\folder name");
                 printf("\n");
                 printf("\n   Modify attributes of specified file or folder,");
                 printf("\n   with the new values entered by the user");
                 printf("\n   Example - chmode temp");
                 printf("\n   Modifies hidden, archive and read-only ");
                 printf("attributes of directory named temp");
                 printf("\n   with the new values taken as input");
                 printf("\n\n");
             }
            else if(strcmpi(temp,"cd")==0)
             {
                 printf("\n - cd Directory Name/path to directory");
                 printf("\n");
                 printf("\n   Changes current directory to the directory ");
                 printf("specified as an argument");
                 printf("\n   Example - cd temp");
                 printf("\n   Changes current directory to temp");
                 printf("\n\n");
             }
            else if(strcmpi(temp,"cd..")==0)
             {
                 printf("\n - cd..");
                 printf("\n Changes current directory to its parent directory");
                 printf("\n\n");
             }
            else if(strcmpi(temp,"cd\\")==0)
             {
                 printf("\n - cd\\");
                 printf("\n   Changes current directory to root directory");
                 printf("\n\n");
             }
            else if(strcmpi(temp,"format")==0)
             {
                 printf("\n - FORMAT");
                 printf("\n   Formats existing disk, removing entire ");
                 printf("data present in it");
                 printf("\n\n");
             }
            else if(strcmpi(temp,"setP")==0)
             {
                 printf("\n - setP File Name");
                 printf("\n");
                 printf("\n   sets the password of the file specified ");
                 printf("as argument");
                 printf("\n   to password entered by user. The file must ");
                 printf("not have a password set already");
                 printf("\n   Example - setP xyz.txt");
                 printf("\n   Sets Password of previously unprotected file ");
                 printf("xyz.txt");
                 printf("\n\n");
             }
            else if(strcmpi(temp,"editP")==0)
             {
                 printf("\n - editP File Name");
                 printf("\n");
                 printf("\n   edits the password of the file specified ");
                 printf("as argument");
                 printf("\n   to password entered by user. The file must ");
                 printf("have a password set already");
                 printf("\n   Example - editP xyz.txt");
                 printf("\n   Edits Password of previously protected file ");
                 printf("xyz.txt");
                 printf("\n\n");
             }
             else if(strcmpi(temp,"remP")==0)
              {
                  printf("\n - remP File Name");
                  printf("\n");
                  printf("\n   removes the password of the file specified ");
                  printf("as argument");
                  printf("\n   The file must have a password set already");
                  printf("\n   Example - remP xyz.txt");
                  printf("\n   Removes Password of previously protected file ");
                  printf("xyz.txt");
                  printf("\n\n");
              }
             else if(strcmpi(temp,"del")==0)
               {
                  printf("\n - del File/Folder Name");
                  printf("\n");
                  printf("\n   sends the file/folder specified as argument");
                  printf("\n   to the recycle bin");
                  printf("\n   Example - del temp");
                  printf("\n   sends temp folder to recycle bin");
                  printf("\n\n");
               }
              else if(strcmpi(temp,"bin:")==0)
               {
                   printf("\n - bin:");
                   printf("\n");
                   printf("\n   switches control to the recycler, ");
                   printf("which operates");
                   printf("\n   the recycle bin");
                   printf("\n\n"); 
               }   
              else if(strcmpi(temp,"size")==0)
               {
                   printf("\n - size");
                   printf("\n");
                   printf("\n   prints used and free disk space in the disk");
                   printf("\n\n");  
               }
         }
}

/****************************************************************************/
int startsWith(char input[],char str[])
{
  int i=0;
  while(str[i]!='\0')
  {
        if(input[i]!=str[i])
           return 0;
        i++;
  }
  return 1;
}  
/****************************************************************************/
int main()
{
    FILE *fp;
    long i;
    int curdir,tcurdir;
    int *dummy;
    int func;
    int flag;
    int current_size_of_bin = 0;
    
    char* count;
    int tempcurdir;
    char name_d[11];
    char input[50];
    char path[512];
    char *temp;
    char *temp1,*temp2;
    struct dir_entry *tmpr = (struct dir_entry*) malloc(DEPS);
    char c;
    printf("\n\t\t\tIMPLEMENTATION OF FILE SYSTEM");
    printf("\n***********************************************");
    printf("*********************************");
    printf("\nWhat do you want to do?\n");
    printf("\n1 Create disk\n");
    printf("2 Use existing disk\n");
    printf("\nPress 1 or 2: ");
    scanf("%d",&flag);
    fflush(stdin);
    fp = fopen("mydisk", "r+b");
    
    while(1)
    {
      if ((fp==NULL)||(flag==1))
      {
         if (fp==NULL)
            printf("\n\nCreating DISK........\n");
         else
         {
            printf("\nSince disk already exists, it will be ");
            printf("formatted\nFomatting DISK.........\n");
         }
            
         fp = fopen("mydisk","w+b");
         for (i=1;i<NO_OF_SECTORS*SIZE_OF_SECTOR;i++)
           putc('\0',fp);
         putc(EOF,fp);
         //initialization.......
         curdir = initialize_disk(fp,curdir);
         break;
      }
      else if(flag==2)
      {
        readSector(fp,mbr,0);
        curdir = buf.mbr_buffer.start_of_root;
        tos = -1;
        push(curdir);
        break;
      }
      else
      {
        printf("\nInvalid input!! Press 1 or 2: ");
        scanf("%d",&flag);
        fflush(stdin);
        fp = fopen("mydisk", "r+b");
      }
    }
     
    printf("\n\nType HELP to get a list of commands");
    printf("\nType EXIT to exit from program");
    printf("\n\n");
    traversal(fp);
    
    while(1)
    {
         fflush(stdin);
         gets(input);
         fflush(stdin);
         if(startsWith(input,"md "))
            func = 1;
         else if(startsWith(input,"cd.."))
            func = 2;
         else if(startsWith(input,"cd "))
            func = 3;
         else if(strcmpi(input,"cd\\")==0)
            func = 4;
         else if(startsWith(input,"mk "))
            func = 5;
         else if(strcmpi(input,"ls")==0)
            func = 6;
         else if(startsWith(input,"rm "))
            func = 7;
         else if(startsWith(input,"sdel "))
            func = 8;
         else if(startsWith(input,"open "))
            func = 9;
         else if(startsWith(input,"chmode "))
            func = 10;
          else if(startsWith(input,"append "))
            func = 11;
         else if(startsWith(input,"overwrite "))
            func = 12;
         else if(startsWith(input,"cut "))
            func = 13;
         else if(startsWith(input,"copy "))
            func = 14;
         else if(strcmpi(input,"paste")==0)
            func = 15;
         else if((strcmpi(input,"help")==0)||(startsWith(input,"help "))
                                                 ||(startsWith(input,"HELP ")))
            func = 16;
         else if(strcmpi(input,"format")==0)
            func = 17;
         else if(startsWith(input,"setP "))
            func = 18;
         else if(startsWith(input,"editP "))
            func = 19;
         else if(startsWith(input,"remP "))
            func = 20;
         else if(startsWith(input,"del "))
            func = 21;
         else if(strcmpi(input,"bin:")==0)
            func = 22;
         else if(strcmpi(input,"size")==0)
            func = 23;
         else if(strcmpi(input,"exit")==0)
            func = 30;
         else
            func = 0;
    
         switch(func)
         {
              case 1: if(!read_only(curdir,fp))
                      {
                        printf("\nCurrent_directory is read only!!");
                        printf("\nNo files and folders can be created here!!");
                       }
                      else
                      {
                        temp = &input[3];
                        int i = strlen(temp);
                        char* ends;
                        if (i>=4)
                        {
                           ends = &temp[i-4];
                           if (strcmpi(ends,".txt")==0)
                           {
                              printf("Sorry, the .txt extension is for a file");
                              break;
                           }
                        }
                        if (strlen(temp)>=NAME_SIZE)
                        {
                           printf("Too long a directory name\n");
                           break;
                        }
                        i=0;
                        flag = 0;
                        while (temp[i]!='\0')
                        {
                            if (temp[i]!=' '&&temp[i]!='\t')
                                  flag = 1;
                            i++;
                        }
                        if (flag==0)
                             temp = "NewFolder";

                        createDir(curdir,temp,fp);
                        pop();
                      }
                      break;
              case 2: curdir = cd_parent(fp,curdir);
                      break;
              case 3: temp = &input[3]; 
                      if((*temp == ' ')||(*temp == '\0'))
                      {
                         printf("\nBad command or file name");
                         break;
                      }
                      curdir = changePath(temp,curdir,fp);
                      break;
              case 4: curdir = cd_root(fp,curdir);
                      break;
              case 5: temp = &input[3];
                      if(!read_only(curdir,fp))
                      {
                        printf("\nCurrent directory is read only!!");
                        printf("\nNo files and folders can be created here!!");
                      }
                      else
                      {
                             i=0;
                             flag = 0;
                             while (temp[i]!='\0')
                             {
                                   if (temp[i]!=' '&&temp[i]!='\t')
                                      flag = 1;
                                   i++;
                             }
                             if (flag==0)
                                temp = "Newf.txt";
                             else
                             {
                                 if (strlen(temp)>6)
                                 {
                                    printf("Too long a fyl name!\n");
                                    break;
                                 }
                                temp = strcat(temp,".txt");
                             }
                             createFile(temp,curdir,fp);
                      }
                      break;
              case 6: list_contents(curdir,fp);
                      break;
              case 7: temp = &input[3];
                      if (*temp=='\0')
                      {
                         printf("no file specified to be renamed\n");
                         break;
                      }
                      temp1 = strtok(temp," ");
                      temp2 = strtok(NULL," ");
                      if (temp2==NULL)
                      {
                         printf("no name specified\n");
                         break;
                      }
                      if(strlen(temp2)>NAME_SIZE - 1)
                      {
                          printf("\n new file name too long!!");
                          break;
                      }
                      rename_file(temp1,temp2,curdir,fp);
                      break;
              case 8: temp = &input[5];
                      del_f(temp,curdir,tmpr,fp);
                      break;
              case 9: temp = &input[5];
                      open_file(temp,curdir,fp);
                      break;
              case 10:temp = &input[7];
                      modify_attr(temp,curdir,fp);
                      break;
              case 11:temp = &input[7];
                      tempcurdir = curdir;
                      dummy = &curdir;
                      if(!(accessFile(temp,curdir,dummy,fp)))
                           break;  
                      curdir = tempcurdir; 
                      append(temp,curdir,fp);
                      printf("\n");
                      break;
              case 12:temp = &input[10];
                      tempcurdir = curdir;
                      dummy = &curdir;
                      if(!(accessFile(temp,curdir,dummy,fp)))
                          break;
                      curdir = tempcurdir;
                      overwrite(temp,curdir,fp);
                      printf("\n"); 
                      break;
              case 13:temp = &input[4];
                      cut(tmpr,temp,curdir,fp);
                      break;
              case 14:temp = &input[5];
                      copy(tmpr,temp,curdir,fp);
                      break;
              case 15:temp = &input[6];
              
                      if(!read_only(curdir,fp))
                      {
                        printf("\nCurrent directory is read only!!");
                        printf("\nNo files and folders can be pasted here!!");
                      }
                      tcurdir = curdir;
                      dummy = (int*)malloc(sizeof(int));
                      paste(tmpr,&tcurdir,dummy,fp);
                      tcurdir = curdir;
                      break;
              case 16:temp = &input[4];
                      if((*temp=='\t')||(*temp=='\0'))
                        display_manual(temp,0);
                      else 
                        {
                           temp = &input[5];
                           display_manual(temp,1);
                        }
                      break;
              case 17:printf("Fomatting DISK.........\n");
                      fp = fopen("mydisk","w+b");
                      for (i=1;i<NO_OF_SECTORS*SIZE_OF_SECTOR;i++)
                         putc('\0',fp);
                      putc(EOF,fp);
                       //initialization.......
                      curdir = initialize_disk(fp,curdir);
                      break;
              case 18:temp = &input[5];
                      setPass(temp,curdir,fp);
                      break;
              case 19:temp = &input[6];
                      editPass(temp,curdir,fp);
                      break;
              case 20:temp = &input[5];
                      removePass(temp,curdir,fp);
                      break;
              case 21:temp = &input[4];
                      sendToBin(&current_size_of_bin,temp,curdir,tmpr,fp);
                      break;
              case 22:recycler(tmpr,fp);
                      curdir = cd_root(fp,curdir);
                      break;
              case 23:display_size(fp);
                      break;
              case 30:printf("\n\n MADE BY: ");
                      printf("\n Anuhbav Gupta ");
                      printf("\n Arpit Gupta  ");
                      printf("\n Anurag Atri   ");
                      printf("\n");
                      
                      getch();
                      fclose(fp);
                      exit(0);
              default:printf("\nBad command or file name\n");
         } 
          printf("\n");
          traversal(fp);
    }        
}
