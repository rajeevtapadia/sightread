// Simple MIDI monitor for Linux (ALSA)
// Build with:
//   gcc -o midi_monitor midi_monitor.c -lasound
//
// Usage:
//   1. List ALSA raw MIDI ports:
//        aplaymidi -l
//      or ALSA sequencer ports:
//        aseqdump -l
//   2. Run this program and pick the port index when prompted.
//
// It will print raw MIDI bytes received from the selected input port.

#include <alsa/asoundlib.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static volatile sig_atomic_t keep_running = 1;

static void handle_sigint(int sig) {
    (void)sig;
    keep_running = 0;
}

static void list_rawmidi_ports(void) {
    int status;
    snd_ctl_t *ctl;
    snd_rawmidi_info_t *info;
    int card = -1;

    snd_rawmidi_info_alloca(&info);

    printf("Available ALSA raw MIDI input ports:\n");

    if ((status = snd_card_next(&card)) < 0 || card < 0) {
        printf("  (no sound cards found)\n");
        return;
    }

    while (card >= 0) {
        char name[32];
        sprintf(name, "hw:%d", card);

        if ((status = snd_ctl_open(&ctl, name, 0)) < 0) {
            fprintf(stderr, "Cannot open control for card %d: %s\n", card, snd_strerror(status));
            goto next_card;
        }

        int device = -1;
        while (1) {
            if ((status = snd_ctl_rawmidi_next_device(ctl, &device)) < 0) {
                fprintf(stderr, "snd_ctl_rawmidi_next_device error: %s\n", snd_strerror(status));
                break;
            }
            if (device < 0)
                break;

            snd_rawmidi_info_set_device(info, device);
            snd_rawmidi_info_set_subdevice(info, 0);
            snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);

            status = snd_ctl_rawmidi_info(ctl, info);
            if (status < 0) {
                // No input on this device, skip
                continue;
            }

            int subs = snd_rawmidi_info_get_subdevices_count(info);
            for (int sub = 0; sub < subs; sub++) {
                snd_rawmidi_info_set_subdevice(info, sub);
                status = snd_ctl_rawmidi_info(ctl, info);
                if (status < 0)
                    continue;

                const char *id = snd_rawmidi_info_get_id(info);
                const char *name_long = snd_rawmidi_info_get_name(info);
                unsigned int card_num = snd_rawmidi_info_get_card(info);
                unsigned int dev_num = snd_rawmidi_info_get_device(info);
                unsigned int sub_num = snd_rawmidi_info_get_subdevice(info);

                printf("  [%d] hw:%u,%u,%u  %s - %s\n",
                       card_num * 100 + dev_num * 10 + sub_num,
                       card_num, dev_num, sub_num,
                       id ? id : "(no id)",
                       name_long ? name_long : "(no name)");
            }
        }

        snd_ctl_close(ctl);

    next_card:
        if ((status = snd_card_next(&card)) < 0) {
            fprintf(stderr, "snd_card_next error: %s\n", snd_strerror(status));
            break;
        }
    }

    printf("\nPick the index in brackets when you run the program.\n\n");
}

int main(int argc, char *argv[]) {
    snd_rawmidi_t *handle_in = NULL;
    int status;
    char device[64];

    signal(SIGINT, handle_sigint);

    printf("Linux ALSA MIDI monitor\n");
    printf("-----------------------\n\n");

    if (argc == 2 && strcmp(argv[1], "--list") == 0) {
        list_rawmidi_ports();
        return 0;
    }

    printf("Tip: Run with '--list' to see available raw MIDI input ports.\n");
    printf("Example device strings: 'hw:1,0,0' or 'hw:1,0'\n\n");

    if (argc >= 2) {
        strncpy(device, argv[1], sizeof(device) - 1);
        device[sizeof(device) - 1] = '\0';
    } else {
        printf("Enter ALSA raw MIDI input device (e.g. hw:1,0,0): ");
        if (!fgets(device, sizeof(device), stdin)) {
            fprintf(stderr, "Failed to read device string\n");
            return 1;
        }
        // Strip newline
        char *nl = strchr(device, '\n');
        if (nl)
            *nl = '\0';
    }

    printf("Opening raw MIDI input on '%s'...\n", device);
    status = snd_rawmidi_open(&handle_in, NULL, device, SND_RAWMIDI_NONBLOCK);
    if (status < 0) {
        fprintf(stderr, "Cannot open raw MIDI device '%s': %s\n", device, snd_strerror(status));
        return 1;
    }

    printf("Opened. Waiting for MIDI data. Press Ctrl+C to quit.\n\n");

    unsigned char buffer[3];

    while (keep_running) {
        int n = snd_rawmidi_read(handle_in, buffer, sizeof(buffer));
        if (n == -EAGAIN) {
            // No data right now, just sleep briefly
            struct timespec ts = {0, 5 * 1000 * 1000}; // 5 ms
            nanosleep(&ts, NULL);
            continue;
        } else if (n < 0) {
            fprintf(stderr, "MIDI read error: %s\n", snd_strerror(n));
            break;
        }

        printf("Read %d byte(s):", n);
        for (int i = 0; i < n; i++) {
            printf(" 0x%02X", buffer[i]);
        }
        printf("\n");
        fflush(stdout);
    }

    printf("Shutting down...\n");
    if (handle_in)
        snd_rawmidi_close(handle_in);
    snd_config_update_free_global();

    return 0;
}



