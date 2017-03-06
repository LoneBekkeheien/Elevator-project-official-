#include "Elevmodule.h"
#include "Timer.h"
#include "Eventmanager.h"
#include "Queue.h"
#include "Statemachine.h"
#include <stdio.h>


enum state{
    IDLE=0,
    RUN = 1,
    EMERGENCY_STOP = 2,
    DOOR_OPEN = 3,
} state, nextstate;

static void elevator_init() {
    while(elev_get_floor_sensor_signal() == -1) {
        elev_set_motor_direction(DIRN_UP);
    }
    elev_set_motor_direction(DIRN_STOP);
    printf("Heisen er initialisert \n");
}

void fsm_EMERGENCY_STOP() {
    elev_set_stop_lamp(1); 
    elev_set_motor_direction(DIRN_STOP); 
    if(elev_get_floor_sensor_signal() != -1){
        elev_set_floor_indicator(elev_get_floor_sensor_signal()); //setter floorindikator KUN hvis vi er i en etajse, hvis ikke vil -1 returneres, og elev_set_ floor ... kræsjer
    }
}

void fsm_IDLE() {
    update_queue();
    queue_add_order();
    print_queue();

    elev_set_motor_direction(DIRN_STOP); 
    if(elev_get_floor_sensor_signal() != -1){ //stopp i etasje
        elev_set_button_lamp(BUTTON_COMMAND, elev_get_floor_sensor_signal(), 0);
        if(elev_get_floor_sensor_signal() != 3){
            elev_set_button_lamp(BUTTON_CALL_UP, elev_get_floor_sensor_signal(), 0);
        }
        if(elev_get_floor_sensor_signal() != 0){
            elev_set_button_lamp(BUTTON_CALL_DOWN, elev_get_floor_sensor_signal(), 0);

        }
        elev_set_floor_indicator(elev_get_floor_sensor_signal());
    }    

}

void fsm_RUN() {
    update_queue();
    queue_add_order();
    print_queue();

    printf("heisen kjorer \n");
    elev_set_motor_direction(direction()); 
}

void fsm_DOOR_OPEN() {
    elev_set_door_open_lamp(1);
    timer_start(); 
     
}


int main() {
    // Initialize hardware
    if (!elev_init()) {
        printf("Unable to initialize elevator hardware!\n");
        return 1;
    }

    printf("Press STOP button to stop elevator and exit program.\n");

    elevator_init(); //initialiserer heisen (kjører opp til nærmeste etasje hvis den ikke allerede står i etasje)
    elev_set_floor_indicator(elev_get_floor_sensor_signal()); 
    queue_init(); //initialiserer køen 
    nOrders_init(); ///initialiserer antall bestillinger (nOrders)
    print_queue(); //printer den initialiserte bestillingslisten
    printf("Antall bestillinger i køen: %d\n", nOrders ); 

    state=IDLE;
    nextstate=0;

    print_queue();
    
    while (1) {

        update_queue(); 
        //double last_floor = elev_get_floor_sensor_signal();
        
        if(elev_get_floor_sensor_signal() >= 0){
            elev_set_floor_indicator(elev_get_floor_sensor_signal()); //setter floorindikator KUN hvis vi er i en etajse, hvis ikke vil -1 returneres, og elev_set_ floor ... kræsjer
        }
        

        switch(state){
                    case IDLE:
                       if(ev_emergency_button_pushed()==1){
                            printf("Emergencybutton pushed, STOP \n");
                            nextstate = EMERGENCY_STOP;
                        }
                        else if(ev_order_same_floor() == 1){
                            printf("Bestilling i etasjen vi er i, DOOR OPEN \n");    
                            nextstate = DOOR_OPEN;
                        }
                        else if(ev_check_buttons()==1){
                            printf("En bestillingsnapp er trykket inn, RUN \n");
                            nextstate = RUN;
                        }
                        else if(nOrders!=0){ //INN I RUN HVIS DET LIGGER INNE BESTILLINGER MEN IKKE AKKURAT I DET EN KNAPP TRYKKES INN 
                            nextstate = RUN;
                        }
                        else{
                            nextstate = IDLE; //hvis ingenting skjer skal heisen bare stå stille 
                        }
                        break;
                    case EMERGENCY_STOP:                        
                        /*
                        if(elev_get_floor_sensor_signal() = -1){
                            if(orders[0].direction == 1){ //går opp
                                last_floor += 0.5;
                            }
                            if(orders[0].direction == -1){ //går ned
                                last_floor -= 0.5;
                            }
                        }
                        */

                        if(ev_emergency_button_pushed()==1){
                            printf("Emergencybutton pushed, STOP \n");
                            nextstate = EMERGENCY_STOP;
                        }
                        if(ev_emergency_button_pushed()==0){
                            if(elev_get_floor_sensor_signal() != -1){ //er i en etasje
                                printf("Emergencybutton pushed in floor, DOOR OPEN\n");
                                queue_delete_all_orders();
                                elev_set_stop_lamp(0);
                                elev_set_floor_indicator(elev_get_floor_sensor_signal()); //kanskje denne må inn i ev_door_open()
                                nextstate = DOOR_OPEN;
                            }
                            else{
                                printf("Emergencybutton pushed, not in floor, IDLE \n");
                                queue_delete_all_orders();
                                elev_set_stop_lamp(0);
                                //hvilken floorindikator skal settes her når man står mellom to etasjer?
                                nextstate = IDLE; 
                            }
                        }
                        break;
                    case RUN:
                        if(ev_order_same_floor() == 0){
                            update_queue();
                            queue_add_order();
                            nextstate = RUN;
                            break;
                        }
                        if(ev_order_same_floor() == 1){
                            update_queue();
                            queue_add_order();
                            printf("order same floor, IDLE \n");
                            nextstate = IDLE;
                            break;
                        }
                        if(ev_emergency_button_pushed() == 1){
                            printf("Emergencybutton pushed, STOP \n");
                            nextstate = EMERGENCY_STOP;
                            break;
                        }
                        break;
                    case DOOR_OPEN:
                        if(ev_time_door_out() == 0){
                            nextstate = DOOR_OPEN;
                        }
                        else{
                            printf("tiden er ute for door, IDLE\n");
                            update_queue();
                            elev_set_door_open_lamp(0); //LUKKER DØR NÅR TIDEN ER UTE
                            queue_delete_order();
                            nextstate = IDLE;
                        }
                        break;
                    default:
                        break;
        }

        if(ev_emergency_button_pushed() == 1){
            nextstate = EMERGENCY_STOP;
        }
        

        if(state != nextstate){
            switch(nextstate){
                case IDLE:
                    fsm_IDLE();
                    break;
                case EMERGENCY_STOP:
                    fsm_EMERGENCY_STOP();
                    break;
                case RUN:   
                    fsm_RUN();
                    break;
                case DOOR_OPEN:
                    fsm_DOOR_OPEN();
                    break;
                default:
                    break;
            }
        }
        
        state = nextstate;
    
    }
    return 0;
}
