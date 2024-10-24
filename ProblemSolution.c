#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

int nrfuels;
int nrcoolant;
pthread_t* tf;
pthread_t* tc;
int critical_temp;
int room_temp;
int generator_temp;
pthread_mutex_t m;
pthread_mutex_t m2;
pthread_cond_t cc;
pthread_cond_t cf;

typedef struct fuel_pump{

        int capacity;
        int feed;
        int pg;
        int hg;

}fuel_pump;


typedef struct cool_pump {
        int capacity;
        int feed;
        int hr;
}cool_pump;



void* fuel(void* arg) {

        int power_generated = 0;
        fuel_pump pump = *(fuel_pump*)arg;

        while(pump.capacity > 0) {
                pthread_mutex_lock(&m);

                while(pump.capacity != 0 && nrcoolant > 0 && generator_temp >= critical_temp) {
                        pthread_cond_wait(&cf, &m);
                }

                if(generator_temp < critical_temp) {
                        generator_temp += pump.hg * pump.feed;
                        power_generated += pump.pg * pump.feed;
                        pump.capacity-=pump.feed;
                        printf("Increased by %d now %d\n", pump.hg * pump.feed, generator_temp);

                }
                if(generator_temp >= critical_temp){
                        printf("Pump generated %d power, remaining fuel %d\n", power_generated, pump.capacity);
                        if(nrcoolant > 0) {
                                pthread_cond_broadcast(&cc);
                                pthread_cond_wait(&cf, &m);
                        }

                }

                pthread_mutex_unlock(&m);

        }

        printf("Fuel pump with feed %d died\n", pump.feed);
        nrfuels--;//critical
        return NULL;
}


void* cool(void* arg) {

        cool_pump pump = *(cool_pump*)arg;

        while(pump.capacity > 0) {
                pthread_mutex_lock(&m2);

                while(pump.capacity != 0 && nrfuels > 0 && generator_temp < critical_temp) {
                        pthread_cond_wait(&cc, &m2);
                }

                if(generator_temp > room_temp) {
                        pump.capacity-=pump.feed;
                        generator_temp -= pump.feed * pump.hr;
                        printf("Decreased by %d now %d\n", pump.feed * pump.hr, generator_temp);
                }
                if(generator_temp < critical_temp){

                        printf("Passing back to fuel\n");
                        if(nrfuels > 0) {
                                pthread_cond_broadcast(&cf);
                                pthread_cond_wait(&cc, &m2);
                        }

                }

                pthread_mutex_unlock(&m2);

        }

        printf("Coolant pump with feed %d died\n", pump.feed);
        nrcoolant--;
        return NULL;
}


int main(int argc, char** argv) {

        room_temp = atoi(argv[1]);
        critical_temp = atoi(argv[2]);
        nrfuels = atoi(argv[3]);
        nrcoolant = atoi(argv[4]);
        generator_temp = room_temp;

        int i,j;
        fuel_pump* fpumps = (fuel_pump*)malloc(sizeof(fuel_pump) * nrfuels);
        cool_pump* cpumps = (cool_pump*)malloc(sizeof(cool_pump) * nrcoolant);
        pthread_mutex_init(&m, NULL);
        pthread_mutex_init(&m2, NULL);
        pthread_cond_init(&cc, NULL);
        pthread_cond_init(&cf, NULL);

        j = 5;
        for(i=0; i<nrfuels; i++){
                fpumps[i].capacity = atoi(argv[j]);
                fpumps[i].feed = atoi(argv[j+1]);
                fpumps[i].pg = atoi(argv[j+2]);
                fpumps[i].hg = atoi(argv[j+3]);
                j+=4;
        }


        for(i=0; i<nrcoolant; i++){
                cpumps[i].capacity = atoi(argv[j]);
                cpumps[i].feed = atoi(argv[j+1]);
                cpumps[i].hr = atoi(argv[j+2]);
                j+=3;
        }

        tf = (pthread_t*)malloc(sizeof(pthread_t) * nrfuels);
        tc = (pthread_t*)malloc(sizeof(pthread_t) * nrcoolant);


        for(i=0; i<nrfuels; i++) {
                pthread_create(&tf[i], NULL, fuel, (void*)&fpumps[i]);
        }

        for(i=0; i<nrcoolant; i++){
                pthread_create(&tc[i], NULL, cool, (void*)&cpumps[i]);
        }

        for(i=0; i<nrfuels; i++)
                pthread_join(tf[i], NULL);

        for(i=0; i<nrcoolant; i++)
                pthread_join(tc[i], NULL);


        pthread_cond_destroy(&cc);
        pthread_cond_destroy(&cf);
        pthread_mutex_destroy(&m);
        pthread_mutex_destroy(&m2);
        free(tf);
        free(tc);
        free(fpumps);
        free(cpumps);

        return 0;
}
