//
//  osxcpuinfo.c
//  osxcpuinfo
//
//  Created by Nándor Krácser on 6/9/14.
//  Copyright (c) 2014 Nándor Krácser. All rights reserved.
//

#include <sys/systm.h>
#include <mach/mach_time.h>
#include <mach/mach_types.h>

kern_return_t osxcpuinfo_start(kmod_info_t *ki, void *d);
kern_return_t osxcpuinfo_stop(kmod_info_t *ki, void *d);

// from kern unsupported
extern void mp_rendezvous(void (*setup_func)(void *),
                          void (*action_func)(void *),
                          void (*teardown_func)(void *),
                          void *arg);

extern int cpu_number(void);

// TODO need to query this
#define NCPUS 4

static void cpuid(int op1, int op2, int *data)
{
    asm volatile("cpuid"
                 : "=a" (data[0]), "=b" (data[1]), "=c" (data[2]), "=d" (data[3])
                 : "a"(op1), "c"(op2));
}

static uint64_t rdtsc()
{
    uint64_t hi, lo;
    asm volatile("rdtsc" : "=a" (lo), "=d" (hi));
    return (hi << 32) | lo;
}

typedef struct
{
    int os_id;
    int socket;
    int core;
    int apic;
} cpu_info_t;

typedef struct
{
    uint64_t raw;
    uint64_t corrected;
} tsc_t;

static void cpu_info_callback(void *data)
{
    int values[4];
    cpu_info_t *cpus = (cpu_info_t *) data;
    int cpu = cpu_number();

    cpus[cpu].os_id = cpu;

    cpuid(0xB, 0, values);
    cpus[cpu].core = values[3] >> values[0] & 0xF;

    cpuid(0xB, 1, values);
    cpus[cpu].socket = values[3] >> values[0] & 0xF;

    cpus[cpu].apic = values[3];
}

static void get_cpu_info_data()
{
    cpu_info_t values[NCPUS];

    mp_rendezvous(NULL, (void (*)(void *))cpu_info_callback,
                  NULL, (void *) values);

    printf("---- get_cpu_info_data ----\n");

    for (int i = 0; i < NCPUS; i++)
    {
        printf("-- cpu_info_t: os_id %d socket %d, core %d, apic %d\n",
               values[i].os_id, values[i].socket, values[i].core, values[i].apic);
    }
}

static void get_cpu_tick_diff_callback(void *data)
{
    tsc_t *values = (tsc_t *) data;
    int cpu = cpu_number();
    values[cpu].raw = rdtsc();
    values[cpu].corrected = mach_absolute_time();
}

static void get_cpu_tick_diff()
{
    // to get nanos:
    // static mach_timebase_info_data_t timebase_info;
    // mach_timebase_info(&timebase_info);
    tsc_t values[NCPUS];

    mp_rendezvous(NULL, get_cpu_tick_diff_callback, NULL, (void *) values);

    printf("---- get_cpu_tick_diff ----\n");

    for (int i = 0; i < NCPUS; i++)
    {
        printf("-- cpu: %d raw: %llu corrected: %llu --\n", i, values[i].raw, values[i].corrected);
    }
}

kern_return_t osxcpuinfo_start(kmod_info_t *ki, void *d)
{
    get_cpu_info_data();
    get_cpu_tick_diff();

    return KERN_SUCCESS;
}

kern_return_t osxcpuinfo_stop(kmod_info_t *ki, void *d)
{
    return KERN_SUCCESS;
}
