#ifndef INC_SCHEDULER_H_
#define INC_SCHEDULER_H_

#include <stdint.h>

typedef struct {
    void (*pTask)(void); // con tro toi ham Task
    uint32_t Delay; // so tick con lai
    uint32_t Period; // chu ky, 0 = one-shot task
    uint8_t  RunMe; // co bao task chay san sang 
    uint32_t TaskID;
} sTask;

#define SCH_MAX_TASKS 40
#define NO_TASK_ID 0

#define SCH_TICK_MS 10
#define ERROR_SCH_TOO_MANY_TASKS 1
#define ERROR_SCH_CANNOT_DELETE_TASK 2

extern sTask SCH_tasks_G[SCH_MAX_TASKS];

void SCH_Init(void);
uint32_t SCH_Add_Task(void(*pFunc)(void), uint32_t DELAY, uint32_t PERIOD);
uint8_t SCH_Delete_Task(uint32_t taskID);
void SCH_Update(void);  // goi trong ngat timer
void SCH_Dispatch_Tasks(void); // goi trong while(1)

void SCH_Go_To_Sleep(void);
void SCH_Report_Status(void);
uint32_t get_time(void);



#endif /* INC_SCHEDULER_H_ */