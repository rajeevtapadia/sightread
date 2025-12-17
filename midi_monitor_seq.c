// Simple MIDI monitor for Linux using ALSA Sequencer API
// Build with:
//   gcc -o midi_monitor_seq midi_monitor_seq.c -lasound
//
// Usage:
//   1. List ALSA sequencer ports:
//        aseqdump -l
//   2. Run this program and either:
//        - pass the client:port string, e.g. "20:0"
//        - or run with no args and it will print available ports and prompt.
//
// It will print MIDI events received from the selected input port.

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

static void list_seq_ports(snd_seq_t *seq) {
    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;

    snd_seq_client_info_alloca(&cinfo);
    snd_seq_port_info_alloca(&pinfo);

    snd_seq_client_info_set_client(cinfo, -1);

    printf("Available ALSA sequencer input ports:\n");

    while (snd_seq_query_next_client(seq, cinfo) >= 0) {
        int client = snd_seq_client_info_get_client(cinfo);

        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);

        while (snd_seq_query_next_port(seq, pinfo) >= 0) {
            unsigned int caps = snd_seq_port_info_get_capability(pinfo);
            unsigned int type = snd_seq_port_info_get_type(pinfo);

            // We want ports we can connect *from* (read from), i.e. they can WRITE to us.
            if (!(caps & SND_SEQ_PORT_CAP_READ) && !(caps & SND_SEQ_PORT_CAP_DUPLEX))
                continue;
            if (!(type & SND_SEQ_PORT_TYPE_MIDI_GENERIC) &&
                !(type & SND_SEQ_PORT_TYPE_APPLICATION))
                continue;

            int port = snd_seq_port_info_get_port(pinfo);
            const char *name = snd_seq_port_info_get_name(pinfo);
            const char *cname = snd_seq_client_info_get_name(cinfo);

            printf("  %d:%d  %s - %s\n", client, port,
                   cname ? cname : "(no client name)",
                   name ? name : "(no port name)");
        }
    }

    printf("\nPick a client:port pair (e.g. 20:0).\n\n");
}

int main(int argc, char *argv[]) {
    snd_seq_t *seq = NULL;
    int in_port = -1;
    int client = -1, port = -1;
    int status;
    char port_str[32];

    signal(SIGINT, handle_sigint);

    printf("Linux ALSA Sequencer MIDI monitor\n");
    printf("---------------------------------\n\n");

    // Open sequencer
    status = snd_seq_open(&seq, "default", SND_SEQ_OPEN_INPUT, 0);
    if (status < 0) {
        fprintf(stderr, "Error opening ALSA sequencer: %s\n", snd_strerror(status));
        return 1;
    }

    snd_seq_set_client_name(seq, "midi_monitor_seq");

    // Create our own input port
    in_port = snd_seq_create_simple_port(seq,
                                         "midi_in",
                                         SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
                                         SND_SEQ_PORT_TYPE_APPLICATION);
    if (in_port < 0) {
        fprintf(stderr, "Error creating sequencer input port: %s\n", snd_strerror(in_port));
        snd_seq_close(seq);
        return 1;
    }

    if (argc >= 2) {
        strncpy(port_str, argv[1], sizeof(port_str) - 1);
        port_str[sizeof(port_str) - 1] = '\0';
    } else {
        list_seq_ports(seq);

        printf("Enter client:port to connect to (e.g. 20:0): ");
        if (!fgets(port_str, sizeof(port_str), stdin)) {
            fprintf(stderr, "Failed to read port string\n");
            snd_seq_close(seq);
            return 1;
        }
        char *nl = strchr(port_str, '\n');
        if (nl)
            *nl = '\0';
    }

    if (sscanf(port_str, "%d:%d", &client, &port) != 2) {
        fprintf(stderr, "Invalid client:port format: '%s'\n", port_str);
        snd_seq_close(seq);
        return 1;
    }

    printf("Connecting from %d:%d to our port %d:%d...\n",
           client, port,
           snd_seq_client_id(seq), in_port);

    status = snd_seq_connect_from(seq, in_port, client, port);
    if (status < 0) {
        fprintf(stderr, "Cannot connect from %d:%d: %s\n",
                client, port, snd_strerror(status));
        snd_seq_close(seq);
        return 1;
    }

    printf("Connected. Waiting for MIDI events. Press Ctrl+C to quit.\n\n");

    snd_seq_event_t *ev;

    while (keep_running) {
        status = snd_seq_event_input(seq, &ev);
        if (status < 0) {
            fprintf(stderr, "Error reading sequencer event: %s\n", snd_strerror(status));
            break;
        }

        if (!ev)
            continue;

        // Basic printout based on event type
        switch (ev->type) {
        case SND_SEQ_EVENT_NOTEON:
            printf("NOTE ON  ch=%d note=%d vel=%d\n",
                   ev->data.note.channel,
                   ev->data.note.note,
                   ev->data.note.velocity);
            break;
        case SND_SEQ_EVENT_NOTEOFF:
            printf("NOTE OFF ch=%d note=%d vel=%d\n",
                   ev->data.note.channel,
                   ev->data.note.note,
                   ev->data.note.velocity);
            break;
        case SND_SEQ_EVENT_CONTROLLER:
            printf("CTRL     ch=%d ctrl=%d value=%d\n",
                   ev->data.control.channel,
                   ev->data.control.param,
                   ev->data.control.value);
            break;
        case SND_SEQ_EVENT_PITCHBEND:
            printf("PITCH    ch=%d value=%d\n",
                   ev->data.control.channel,
                   ev->data.control.value);
            break;
        default:
            printf("OTHER    type=%d\n", ev->type);
            break;
        }

        fflush(stdout);
    }

    printf("Shutting down...\n");
    snd_seq_disconnect_from(seq, in_port, client, port);
    snd_seq_close(seq);
    snd_config_update_free_global();

    return 0;
}



