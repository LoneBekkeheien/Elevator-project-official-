//denne funksjonen gjør at heisen går opp til nærmeste etasje når vi starter programmet (initialiserer).
static void elevator_init();

//setter lampen til 1, og direksjonen til 0.
void fsm_EMERGENCY_STOP();

void fsm_IDLE();

void fsm_RUN();

void fsm_DOOR_OPEN();