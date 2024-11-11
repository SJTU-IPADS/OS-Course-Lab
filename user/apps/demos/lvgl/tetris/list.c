#include <stdlib.h>

typedef struct list_node
{
  int data;
  int cur;
} ListNode;

void empty_list()
{
  link_list[0].cur = 0;
  int i;
  for(i=1; i < MAX_SIZE - 1; i++)
    link_list[i].cur = i+1;
  link_list[MAX_SIZE - 1].cur = 0;
} 
 
int insert_link_list(int data)
{
  if(link_list[1].cur == 0)
    return -1;
  int pos = link_list[1].cur;
  link_list[1].cur = link_list[pos].cur;
  link_list[pos].data = data;
  link_list[pos].cur = link_list[0].cur;
  link_list[0].cur = pos; 
  return 0;
}

int delete_link_list(int *data, int pos)
{
  if(data == NULL || link_list[0].cur == 0 || pos < 1)
    return -1;
  int i = 1;
  int cur = 0;
  while(i<pos && link_list[cur].cur)
  {
    i++;
    cur = link_list[cur].cur;
  }
  
  if(i==pos && link_list[cur].cur)
  {
    int t = link_list[cur].cur;
    *data = link_list[t].data;
    link_list[cur].cur = link_list[t].cur;
    link_list[t].cur = link_list[1].cur;
    link_list[1].cur = t;
  }
  return 0;  
}