#ifndef CONFIG_H
#define CONFIG_H




//For systems with digital trigger in (PFI4 if 4 AI, PFI8 if 8 AI)
#define TRIGGER_IN "/Dev2/PFI8"
#define AI_CHANNELS "/Dev2/ai0:15" //Varies from system to system
#define N_AI_CHANNELS 16 //Must match the number of analog inputs

#define ILLUM_PORT "/Dev2/port0/line0:5" //0:5 on new systems, lower on older system (equal to color number)



#endif // CONFIG_H
