#include "scheduler.h"
#include <stddef.h>



static uint8_t SCH_count = 0;
unsigned char Error_code_G = 0;
static volatile uint32_t system_ticks = 0;
static uint32_t SCH_missed_ticks = 0;
sTask SCH_tasks_G[SCH_MAX_TASKS];
static uint32_t next_id = 1;

static void SCH_Insert_Task(sTask task){
    uint8_t position = 0;
    uint32_t remaining = task.Delay;

    while (position < SCH_count && remaining >= SCH_tasks_G[position].Delay){
        remaining -= SCH_tasks_G[position].Delay;
        position++;
    }

    for (uint8_t i = SCH_count; i > position; i--){
        SCH_tasks_G[i] = SCH_tasks_G[i - 1];
    }
    task.Delay = remaining;
    if (position < SCH_count){
        SCH_tasks_G[position + 1].Delay -= remaining;
    }
    SCH_tasks_G[position] = task;
    SCH_count++;
}



void SCH_Init(void){
    for (uint8_t i = 0; i < SCH_MAX_TASKS; i++){
        SCH_tasks_G[i].pTask = NULL;
        SCH_tasks_G[i].Delay = 0;
        SCH_tasks_G[i].Period = 0;
        SCH_tasks_G[i].RunMe = 0;
        SCH_tasks_G[i].TaskID = 0;
    }
    SCH_count = 0;  
    Error_code_G = 0;
    next_id = 1;
    system_ticks = 0;
    SCH_missed_ticks = 0;
}

uint32_t SCH_Add_Task(void(*pFunc)(void), uint32_t DELAY, uint32_t PERIOD){
    if (pFunc == NULL){
        return NO_TASK_ID;
    }
    uint32_t irq_state = __get_PRIMASK();
    __disable_irq();

    if (SCH_count >= SCH_MAX_TASKS){
        Error_code_G = ERROR_SCH_TOO_MANY_TASKS;
        __set_PRIMASK(irq_state);
        return NO_TASK_ID;
    }

    sTask new_task;
    new_task.pTask = pFunc;
    new_task.Delay = DELAY;
    new_task.Period = PERIOD;
    new_task.RunMe = 0;
    new_task.TaskID = next_id++;

    SCH_Insert_Task(new_task);
    __set_PRIMASK(irq_state);

    return new_task.TaskID;
}

uint8_t SCH_Delete_Task(uint32_t taskID){
    uint8_t result = 0;

    uint32_t irq_state = __get_PRIMASK();
    __disable_irq();

    if (SCH_count == 0 || taskID == 0){
        __set_PRIMASK(irq_state);
        return 0;
    }

    for (uint8_t i = 0; i < SCH_count; i++){
        if (SCH_tasks_G[i].TaskID == taskID){
            if (i + 1 < SCH_count){
                SCH_tasks_G[i + 1].Delay += SCH_tasks_G[i].Delay;
            }
            
            for (uint8_t j = i; j + 1 < SCH_count; j++){
                SCH_tasks_G[j] = SCH_tasks_G[j + 1];
            }
            SCH_count--;
            SCH_tasks_G[SCH_count] = (sTask){0};

            result = 1;
            break;
        }
    }
    if (result == 0){
        Error_code_G =  ERROR_SCH_CANNOT_DELETE_TASK;
    }
    __set_PRIMASK(irq_state);
    return result;
}

void SCH_Update(void){
    system_ticks += SCH_TICK_MS;
    if (SCH_count == 0 || SCH_tasks_G[0].pTask == NULL){
        return;
    }

    if (SCH_tasks_G[0].Delay > 0){
        SCH_tasks_G[0].Delay--;
    }

    if (SCH_tasks_G[0].Delay == 0){
        SCH_tasks_G[0].RunMe = 1;
    }

}


void SCH_Dispatch_Tasks(void){
    sTask current_task;

    uint32_t irq_state = __get_PRIMASK();
    __disable_irq();

    if (SCH_count == 0 || SCH_tasks_G[0].Delay != 0)
    {
        __set_PRIMASK(irq_state);
        return;
    }
    current_task = SCH_tasks_G[0];
    for (uint8_t i = 0; i + 1 < SCH_count; i++)
    {
        SCH_tasks_G[i] = SCH_tasks_G[i + 1];
    }

    SCH_count--;
    SCH_tasks_G[SCH_count] = (sTask){0};

    __set_PRIMASK(irq_state);
    if (current_task.pTask != NULL)
    {
        current_task.pTask();
    }
    if (current_task.Period > 0 &&
        current_task.pTask != NULL)
    {
        current_task.Delay = current_task.Period;
        current_task.RunMe = 0;

        irq_state = __get_PRIMASK();
        __disable_irq();

        if (SCH_count < SCH_MAX_TASKS)
        {
            SCH_Insert_Task(current_task);
        }
        else
        {
            Error_code_G = ERROR_SCH_TOO_MANY_TASKS;
        }
        __set_PRIMASK(irq_state);
    }
}

uint32_t get_time(void) {
    return system_ticks;
}

void SCH_Go_To_Sleep(void) {

}

void SCH_Report_Status(void) {
}

