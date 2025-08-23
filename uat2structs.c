#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uat.h"
#include "uat_decode.h"
#include "reader.h"

// Self-contained downlink test data structure
typedef struct
{
    char frame_data_hex[97]; // 48 bytes * 2 hex chars + null terminator
    int frame_length;
    const char *test_name;

    // Decoded HDR fields
    int mdb_type;
    int address_qualifier;
    uint32_t address;

    // Decoded SV fields
    uint8_t has_sv;
    int nic;
    uint8_t position_valid : 1;
    double lat;
    double lon;
    int altitude_type; // 0=invalid, 1=baro, 2=geo
    int altitude;
    int airground_state;
    uint8_t ns_vel_valid : 1;
    int ns_vel;
    uint8_t ew_vel_valid : 1;
    int ew_vel;
    int track_type; // 0=invalid, 1=track, 2=mag_heading, 3=true_heading
    uint16_t track;
    uint8_t speed_valid : 1;
    uint16_t speed;
    int vert_rate_source; // 0=invalid, 1=baro, 2=geo
    int vert_rate;
    uint8_t dimensions_valid : 1;
    double length;
    double width;
    int position_offset;
    uint8_t utc_coupled : 1;
    int tisb_site_id;

    // Decoded MS fields
    uint8_t has_ms : 1;
    int emitter_category;
    char callsign[9];
    int callsign_type; // 0=invalid, 1=callsign, 2=squawk
    int emergency_status;
    int uat_version;
    int sil;
    int transmit_mso;
    int nac_p;
    int nac_v;
    int nic_baro;
    uint8_t has_cdti : 1;
    uint8_t has_acas : 1;
    uint8_t acas_ra_active : 1;
    uint8_t ident_active : 1;
    uint8_t atc_services : 1;
    uint8_t heading_type : 1; // 0=true, 1=magnetic

    // Decoded AUXSV fields
    uint8_t has_auxsv : 1;
    int sec_altitude_type; // 0=invalid, 1=baro, 2=geo
    int sec_altitude;
} uat_downlink_test_frame_t;

// Self-contained uplink test data structure
typedef struct
{
    char frame_data_hex[1105]; // 552 bytes * 2 hex chars + null terminator
    int frame_length;
    const char *test_name;

    // Decoded uplink fields
    int position_valid;
    double lat;
    double lon;
    int utc_coupled;
    int app_data_valid;
    int slot_id;
    int tisb_site_id;
    int num_info_frames;

    // Info frame data (simplified for testing)
    struct
    {
        int length;
        int type;
        int is_fisb;
        // FISB fields if applicable
        int fisb_product_id;
        int fisb_a_flag;
        int fisb_g_flag;
        int fisb_p_flag;
        int fisb_s_flag;
        int fisb_hours;
        int fisb_minutes;
        int fisb_seconds;
        int fisb_seconds_valid;
        int fisb_month;
        int fisb_day;
        int fisb_monthday_valid;
    } info_frames[8]; // Limit to 8 frames for simplicity
} uat_uplink_test_frame_t;

// Arrays to store test frames
static uat_downlink_test_frame_t downlink_test_frames[1000];
static uat_uplink_test_frame_t uplink_test_frames[1000];
static int num_downlink_frames = 0;
static int num_uplink_frames = 0;

// Helper function to convert bytes to hex string
void bytes_to_hex_string(uint8_t *bytes, int length, char *hex_string)
{
    for (int i = 0; i < length; i++)
    {
        sprintf(hex_string + (i * 2), "%02x", bytes[i]);
    }
    hex_string[length * 2] = '\0';
}

void handle_frame(frame_type_t type, uint8_t *frame, int len, void *extra)
{
    if (type == UAT_DOWNLINK)
    {
        if (num_downlink_frames >= 1000)
        {
            fprintf(stderr, "Warning: Maximum number of downlink test frames exceeded\n");
            return;
        }

        struct uat_adsb_mdb mdb;
        uat_decode_adsb_mdb(frame, &mdb);

        uat_downlink_test_frame_t *test_frame = &downlink_test_frames[num_downlink_frames];
        memset(test_frame, 0, sizeof(*test_frame));

        // Convert frame data to hex string
        bytes_to_hex_string(frame, len, test_frame->frame_data_hex);
        test_frame->frame_length = len;

        // Generate test name
        char test_name[256];
        snprintf(test_name, sizeof(test_name), "Downlink_MDB%d_Addr%06X_%d",
                 mdb.mdb_type, mdb.address, num_downlink_frames);
        test_frame->test_name = strdup(test_name);

        // Copy decoded HDR fields
        test_frame->mdb_type = mdb.mdb_type;
        test_frame->address_qualifier = mdb.address_qualifier;
        test_frame->address = mdb.address;

        // Copy decoded SV fields
        test_frame->has_sv = mdb.has_sv;
        if (mdb.has_sv)
        {
            test_frame->nic = mdb.nic;
            test_frame->position_valid = mdb.position_valid;
            test_frame->lat = mdb.lat;
            test_frame->lon = mdb.lon;
            test_frame->altitude_type = mdb.altitude_type;
            test_frame->altitude = mdb.altitude;
            test_frame->airground_state = mdb.airground_state;
            test_frame->ns_vel_valid = mdb.ns_vel_valid;
            test_frame->ns_vel = mdb.ns_vel;
            test_frame->ew_vel_valid = mdb.ew_vel_valid;
            test_frame->ew_vel = mdb.ew_vel;
            test_frame->track_type = mdb.track_type;
            test_frame->track = mdb.track;
            test_frame->speed_valid = mdb.speed_valid;
            test_frame->speed = mdb.speed;
            test_frame->vert_rate_source = mdb.vert_rate_source;
            test_frame->vert_rate = mdb.vert_rate;
            test_frame->dimensions_valid = mdb.dimensions_valid;
            test_frame->length = mdb.length;
            test_frame->width = mdb.width;
            test_frame->position_offset = mdb.position_offset;
            test_frame->utc_coupled = mdb.utc_coupled;
            test_frame->tisb_site_id = mdb.tisb_site_id;
        }

        // Copy decoded MS fields
        test_frame->has_ms = mdb.has_ms;
        if (mdb.has_ms)
        {
            test_frame->emitter_category = mdb.emitter_category;
            strncpy(test_frame->callsign, mdb.callsign, sizeof(test_frame->callsign));
            test_frame->callsign_type = mdb.callsign_type;
            test_frame->emergency_status = mdb.emergency_status;
            test_frame->uat_version = mdb.uat_version;
            test_frame->sil = mdb.sil;
            test_frame->transmit_mso = mdb.transmit_mso;
            test_frame->nac_p = mdb.nac_p;
            test_frame->nac_v = mdb.nac_v;
            test_frame->nic_baro = mdb.nic_baro;
            test_frame->has_cdti = mdb.has_cdti;
            test_frame->has_acas = mdb.has_acas;
            test_frame->acas_ra_active = mdb.acas_ra_active;
            test_frame->ident_active = mdb.ident_active;
            test_frame->atc_services = mdb.atc_services;
            test_frame->heading_type = mdb.heading_type;
        }

        // Copy decoded AUXSV fields
        test_frame->has_auxsv = mdb.has_auxsv;
        if (mdb.has_auxsv)
        {
            test_frame->sec_altitude_type = mdb.sec_altitude_type;
            test_frame->sec_altitude = mdb.sec_altitude;
        }

        num_downlink_frames++;
    }
    else
    { // UAT_UPLINK
        if (num_uplink_frames >= 1000)
        {
            fprintf(stderr, "Warning: Maximum number of uplink test frames exceeded\n");
            return;
        }

        struct uat_uplink_mdb mdb;
        uat_decode_uplink_mdb(frame, &mdb);

        uat_uplink_test_frame_t *test_frame = &uplink_test_frames[num_uplink_frames];
        memset(test_frame, 0, sizeof(*test_frame));

        // Convert frame data to hex string
        bytes_to_hex_string(frame, len, test_frame->frame_data_hex);
        test_frame->frame_length = len;

        // Generate test name
        char test_name[256];
        snprintf(test_name, sizeof(test_name), "Uplink_Site%d_Slot%d_%d",
                 mdb.tisb_site_id, mdb.slot_id, num_uplink_frames);
        test_frame->test_name = strdup(test_name);

        // Copy decoded uplink fields
        test_frame->position_valid = mdb.position_valid;
        test_frame->lat = mdb.lat;
        test_frame->lon = mdb.lon;
        test_frame->utc_coupled = mdb.utc_coupled;
        test_frame->app_data_valid = mdb.app_data_valid;
        test_frame->slot_id = mdb.slot_id;
        test_frame->tisb_site_id = mdb.tisb_site_id;
        test_frame->num_info_frames = mdb.num_info_frames;

        // Copy info frame data (limit to array size)
        int frames_to_copy = (mdb.num_info_frames > 8) ? 8 : mdb.num_info_frames;
        for (int i = 0; i < frames_to_copy; i++)
        {
            test_frame->info_frames[i].length = mdb.info_frames[i].length;
            test_frame->info_frames[i].type = mdb.info_frames[i].type;
            test_frame->info_frames[i].is_fisb = mdb.info_frames[i].is_fisb;

            if (mdb.info_frames[i].is_fisb)
            {
                test_frame->info_frames[i].fisb_product_id = mdb.info_frames[i].fisb.product_id;
                test_frame->info_frames[i].fisb_a_flag = mdb.info_frames[i].fisb.a_flag;
                test_frame->info_frames[i].fisb_g_flag = mdb.info_frames[i].fisb.g_flag;
                test_frame->info_frames[i].fisb_p_flag = mdb.info_frames[i].fisb.p_flag;
                test_frame->info_frames[i].fisb_s_flag = mdb.info_frames[i].fisb.s_flag;
                test_frame->info_frames[i].fisb_hours = mdb.info_frames[i].fisb.hours;
                test_frame->info_frames[i].fisb_minutes = mdb.info_frames[i].fisb.minutes;
                test_frame->info_frames[i].fisb_seconds = mdb.info_frames[i].fisb.seconds;
                test_frame->info_frames[i].fisb_seconds_valid = mdb.info_frames[i].fisb.seconds_valid;
                test_frame->info_frames[i].fisb_month = mdb.info_frames[i].fisb.month;
                test_frame->info_frames[i].fisb_day = mdb.info_frames[i].fisb.day;
                test_frame->info_frames[i].fisb_monthday_valid = mdb.info_frames[i].fisb.monthday_valid;
            }
        }

        num_uplink_frames++;
    }
}

void output_test_header(FILE *output)
{
    fprintf(output, "// Auto-generated UAT test data for Google Test\n");
    fprintf(output, "// Generated from UAT test frames\n");
    fprintf(output, "// Self-contained - no external dependencies required\n\n");
    fprintf(output, "#include <stdint.h>\n\n");
    fprintf(output, "#include <stddef.h>\n\n");

    fprintf(output, "// Enum value constants\n");
    fprintf(output, "#define UAT_ALT_INVALID 0\n");
    fprintf(output, "#define UAT_ALT_BARO    1\n");
    fprintf(output, "#define UAT_ALT_GEO     2\n\n");

    fprintf(output, "#define UAT_TT_INVALID      0\n");
    fprintf(output, "#define UAT_TT_TRACK        1\n");
    fprintf(output, "#define UAT_TT_MAG_HEADING  2\n");
    fprintf(output, "#define UAT_TT_TRUE_HEADING 3\n\n");

    fprintf(output, "#define UAT_CS_INVALID   0\n");
    fprintf(output, "#define UAT_CS_CALLSIGN  1\n");
    fprintf(output, "#define UAT_CS_SQUAWK    2\n\n");

    fprintf(output, "#define UAT_HT_TRUE      0\n");
    fprintf(output, "#define UAT_HT_MAGNETIC  1\n\n");

    fprintf(output, "// Downlink test frame structure\n");
    fprintf(output, "typedef struct {\n");
    fprintf(output, "    const char* frame_data_hex;\n");
    fprintf(output, "    int frame_length;\n");
    fprintf(output, "    const char* test_name;\n");
    fprintf(output, "    \n");
    fprintf(output, "    // Decoded HDR fields\n");
    fprintf(output, "    int mdb_type;\n");
    fprintf(output, "    int address_qualifier;\n");
    fprintf(output, "    uint32_t address;\n");
    fprintf(output, "    \n");
    fprintf(output, "    // Decoded SV fields\n");
    fprintf(output, "    int has_sv;\n");
    fprintf(output, "    int nic;\n");
    fprintf(output, "    int position_valid;\n");
    fprintf(output, "    double lat;\n");
    fprintf(output, "    double lon;\n");
    fprintf(output, "    int altitude_type;\n");
    fprintf(output, "    int altitude;\n");
    fprintf(output, "    int airground_state;\n");
    fprintf(output, "    int ns_vel_valid;\n");
    fprintf(output, "    int ns_vel;\n");
    fprintf(output, "    int ew_vel_valid;\n");
    fprintf(output, "    int ew_vel;\n");
    fprintf(output, "    int track_type;\n");
    fprintf(output, "    uint16_t track;\n");
    fprintf(output, "    int speed_valid;\n");
    fprintf(output, "    uint16_t speed;\n");
    fprintf(output, "    int vert_rate_source;\n");
    fprintf(output, "    int vert_rate;\n");
    fprintf(output, "    int dimensions_valid;\n");
    fprintf(output, "    double length;\n");
    fprintf(output, "    double width;\n");
    fprintf(output, "    int position_offset;\n");
    fprintf(output, "    int utc_coupled;\n");
    fprintf(output, "    int tisb_site_id;\n");
    fprintf(output, "    \n");
    fprintf(output, "    // Decoded MS fields\n");
    fprintf(output, "    int has_ms;\n");
    fprintf(output, "    int emitter_category;\n");
    fprintf(output, "    char callsign[9];\n");
    fprintf(output, "    int callsign_type;\n");
    fprintf(output, "    int emergency_status;\n");
    fprintf(output, "    int uat_version;\n");
    fprintf(output, "    int sil;\n");
    fprintf(output, "    int transmit_mso;\n");
    fprintf(output, "    int nac_p;\n");
    fprintf(output, "    int nac_v;\n");
    fprintf(output, "    int nic_baro;\n");
    fprintf(output, "    int has_cdti;\n");
    fprintf(output, "    int has_acas;\n");
    fprintf(output, "    int acas_ra_active;\n");
    fprintf(output, "    int ident_active;\n");
    fprintf(output, "    int atc_services;\n");
    fprintf(output, "    int heading_type;\n");
    fprintf(output, "    \n");
    fprintf(output, "    // Decoded AUXSV fields\n");
    fprintf(output, "    int has_auxsv;\n");
    fprintf(output, "    int sec_altitude_type;\n");
    fprintf(output, "    int sec_altitude;\n");
    fprintf(output, "} uat_downlink_test_frame_t;\n\n");

    fprintf(output, "// Uplink test frame structure\n");
    fprintf(output, "typedef struct {\n");
    fprintf(output, "    const char* frame_data_hex;\n");
    fprintf(output, "    int frame_length;\n");
    fprintf(output, "    const char* test_name;\n");
    fprintf(output, "    \n");
    fprintf(output, "    // Decoded uplink fields\n");
    fprintf(output, "    int position_valid;\n");
    fprintf(output, "    double lat;\n");
    fprintf(output, "    double lon;\n");
    fprintf(output, "    int utc_coupled;\n");
    fprintf(output, "    int app_data_valid;\n");
    fprintf(output, "    int slot_id;\n");
    fprintf(output, "    int tisb_site_id;\n");
    fprintf(output, "    int num_info_frames;\n");
    fprintf(output, "    \n");
    fprintf(output, "    // Info frame data\n");
    fprintf(output, "    struct {\n");
    fprintf(output, "        int length;\n");
    fprintf(output, "        int type;\n");
    fprintf(output, "        int is_fisb;\n");
    fprintf(output, "        int fisb_product_id;\n");
    fprintf(output, "        int fisb_a_flag;\n");
    fprintf(output, "        int fisb_g_flag;\n");
    fprintf(output, "        int fisb_p_flag;\n");
    fprintf(output, "        int fisb_s_flag;\n");
    fprintf(output, "        int fisb_hours;\n");
    fprintf(output, "        int fisb_minutes;\n");
    fprintf(output, "        int fisb_seconds;\n");
    fprintf(output, "        int fisb_seconds_valid;\n");
    fprintf(output, "        int fisb_month;\n");
    fprintf(output, "        int fisb_day;\n");
    fprintf(output, "        int fisb_monthday_valid;\n");
    fprintf(output, "    } info_frames[8];\n");
    fprintf(output, "} uat_uplink_test_frame_t;\n\n");
}

void output_downlink_data(FILE *output)
{
    fprintf(output, "// Downlink test frame data array\n");
    fprintf(output, "static const uat_downlink_test_frame_t uat_downlink_test_frames[] = {\n");

    for (int i = 0; i < num_downlink_frames; i++)
    {
        const uat_downlink_test_frame_t *frame = &downlink_test_frames[i];
        fprintf(output, "    {\n");
        fprintf(output, "        // %s\n", frame->test_name);
        fprintf(output, "        \"%s\",  // frame_data_hex\n", frame->frame_data_hex);
        fprintf(output, "        %d,  // frame_length\n", frame->frame_length);
        fprintf(output, "        \"%s\",  // test_name\n", frame->test_name);
        fprintf(output, "        %d, %d, %u,  // HDR: mdb_type, address_qualifier, address\n",
                frame->mdb_type, frame->address_qualifier, frame->address);

        // SV fields
        fprintf(output, "        %d, %d, %d, %.6f, %.6f, %d, %d, %d,  // SV: has_sv, nic, position_valid, lat, lon, altitude_type, altitude, airground_state\n",
                frame->has_sv, frame->nic, frame->position_valid, frame->lat, frame->lon,
                frame->altitude_type, frame->altitude, frame->airground_state);
        fprintf(output, "        %d, %d, %d, %d, %d, %u, %d, %u,  // SV: ns_vel_valid, ns_vel, ew_vel_valid, ew_vel, track_type, track, speed_valid, speed\n",
                frame->ns_vel_valid, frame->ns_vel, frame->ew_vel_valid, frame->ew_vel,
                frame->track_type, frame->track, frame->speed_valid, frame->speed);
        fprintf(output, "        %d, %d, %d, %.1f, %.1f, %d, %d, %d,  // SV: vert_rate_source, vert_rate, dimensions_valid, length, width, position_offset, utc_coupled, tisb_site_id\n",
                frame->vert_rate_source, frame->vert_rate, frame->dimensions_valid, frame->length, frame->width,
                frame->position_offset, frame->utc_coupled, frame->tisb_site_id);

        // MS fields
        fprintf(output, "        %d, %d, \"%s\", %d, %d, %d, %d, %d,  // MS: has_ms, emitter_category, callsign, callsign_type, emergency_status, uat_version, sil, transmit_mso\n",
                frame->has_ms, frame->emitter_category, frame->callsign, frame->callsign_type,
                frame->emergency_status, frame->uat_version, frame->sil, frame->transmit_mso);
        fprintf(output, "        %d, %d, %d, %d, %d, %d, %d, %d, %d,  // MS: nac_p, nac_v, nic_baro, has_cdti, has_acas, acas_ra_active, ident_active, atc_services, heading_type\n",
                frame->nac_p, frame->nac_v, frame->nic_baro, frame->has_cdti, frame->has_acas,
                frame->acas_ra_active, frame->ident_active, frame->atc_services, frame->heading_type);

        // AUXSV fields
        fprintf(output, "        %d, %d, %d  // AUXSV: has_auxsv, sec_altitude_type, sec_altitude\n",
                frame->has_auxsv, frame->sec_altitude_type, frame->sec_altitude);

        fprintf(output, "    }");
        if (i < num_downlink_frames - 1)
        {
            fprintf(output, ",");
        }
        fprintf(output, "\n");
    }

    fprintf(output, "};\n\n");
    fprintf(output, "static const int uat_downlink_test_frames_count = %d;\n\n", num_downlink_frames);
}

void output_uplink_data(FILE *output)
{
    fprintf(output, "// Uplink test frame data array\n");
    fprintf(output, "static const uat_uplink_test_frame_t uat_uplink_test_frames[] = {\n");

    for (int i = 0; i < num_uplink_frames; i++)
    {
        const uat_uplink_test_frame_t *frame = &uplink_test_frames[i];
        fprintf(output, "    {\n");
        fprintf(output, "        // %s\n", frame->test_name);
        fprintf(output, "        \"%s\",  // frame_data_hex\n", frame->frame_data_hex);
        fprintf(output, "        %d,  // frame_length\n", frame->frame_length);
        fprintf(output, "        \"%s\",  // test_name\n", frame->test_name);
        fprintf(output, "        %d, %.6f, %.6f, %d, %d, %d, %d, %d,  // position_valid, lat, lon, utc_coupled, app_data_valid, slot_id, tisb_site_id, num_info_frames\n",
                frame->position_valid, frame->lat, frame->lon, frame->utc_coupled,
                frame->app_data_valid, frame->slot_id, frame->tisb_site_id, frame->num_info_frames);

        // Info frames
        fprintf(output, "        {");
        for (int j = 0; j < 8; j++)
        {
            if (j > 0)
                fprintf(output, ", ");
            fprintf(output, "\n            {%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d}",
                    frame->info_frames[j].length, frame->info_frames[j].type, frame->info_frames[j].is_fisb,
                    frame->info_frames[j].fisb_product_id, frame->info_frames[j].fisb_a_flag,
                    frame->info_frames[j].fisb_g_flag, frame->info_frames[j].fisb_p_flag,
                    frame->info_frames[j].fisb_s_flag, frame->info_frames[j].fisb_hours,
                    frame->info_frames[j].fisb_minutes, frame->info_frames[j].fisb_seconds,
                    frame->info_frames[j].fisb_seconds_valid, frame->info_frames[j].fisb_month,
                    frame->info_frames[j].fisb_day, frame->info_frames[j].fisb_monthday_valid);
        }
        fprintf(output, "\n        }\n");

        fprintf(output, "    }");
        if (i < num_uplink_frames - 1)
        {
            fprintf(output, ",");
        }
        fprintf(output, "\n");
    }

    fprintf(output, "};\n\n");
    fprintf(output, "static const int uat_uplink_test_frames_count = %d;\n\n", num_uplink_frames);
}

void output_accessor_functions(FILE *output)
{
    fprintf(output, "// Accessor functions for test data\n");
    fprintf(output, "const uat_downlink_test_frame_t* get_uat_downlink_test_frames() {\n");
    fprintf(output, "    return uat_downlink_test_frames;\n");
    fprintf(output, "}\n\n");

    fprintf(output, "int get_uat_downlink_test_frames_count() {\n");
    fprintf(output, "    return uat_downlink_test_frames_count;\n");
    fprintf(output, "}\n\n");

    fprintf(output, "const uat_downlink_test_frame_t* get_uat_downlink_test_frame(int index) {\n");
    fprintf(output, "    if (index < 0 || index >= uat_downlink_test_frames_count) {\n");
    fprintf(output, "        return NULL;\n");
    fprintf(output, "    }\n");
    fprintf(output, "    return &uat_downlink_test_frames[index];\n");
    fprintf(output, "}\n\n");

    fprintf(output, "const uat_uplink_test_frame_t* get_uat_uplink_test_frames() {\n");
    fprintf(output, "    return uat_uplink_test_frames;\n");
    fprintf(output, "}\n\n");

    fprintf(output, "int get_uat_uplink_test_frames_count() {\n");
    fprintf(output, "    return uat_uplink_test_frames_count;\n");
    fprintf(output, "}\n\n");

    fprintf(output, "const uat_uplink_test_frame_t* get_uat_uplink_test_frame(int index) {\n");
    fprintf(output, "    if (index < 0 || index >= uat_uplink_test_frames_count) {\n");
    fprintf(output, "        return NULL;\n");
    fprintf(output, "    }\n");
    fprintf(output, "    return &uat_uplink_test_frames[index];\n");
    fprintf(output, "}\n");
}

void output_header_file(const char *header_filename)
{
    FILE *header = fopen(header_filename, "w");
    if (!header)
    {
        perror("Failed to create header file");
        return;
    }

    fprintf(header, "// Auto-generated UAT test data header for Google Test\n");
    fprintf(header, "// Self-contained - no external dependencies required\n");
    fprintf(header, "#ifndef UAT_TEST_DATA_H\n");
    fprintf(header, "#define UAT_TEST_DATA_H\n\n");
    fprintf(header, "#include <stdint.h>\n\n");

    fprintf(header, "// Enum value constants\n");
    fprintf(header, "#define UAT_ALT_INVALID 0\n");
    fprintf(header, "#define UAT_ALT_BARO    1\n");
    fprintf(header, "#define UAT_ALT_GEO     2\n\n");

    fprintf(header, "#define UAT_TT_INVALID      0\n");
    fprintf(header, "#define UAT_TT_TRACK        1\n");
    fprintf(header, "#define UAT_TT_MAG_HEADING  2\n");
    fprintf(header, "#define UAT_TT_TRUE_HEADING 3\n\n");

    fprintf(header, "#define UAT_CS_INVALID   0\n");
    fprintf(header, "#define UAT_CS_CALLSIGN  1\n");
    fprintf(header, "#define UAT_CS_SQUAWK    2\n\n");

    fprintf(header, "#define UAT_HT_TRUE      0\n");
    fprintf(header, "#define UAT_HT_MAGNETIC  1\n\n");

    fprintf(header, "// Downlink test frame structure\n");
    fprintf(header, "typedef struct {\n");
    fprintf(header, "    const char* frame_data_hex;\n");
    fprintf(header, "    int frame_length;\n");
    fprintf(header, "    const char* test_name;\n");
    fprintf(header, "    \n");
    fprintf(header, "    // Decoded HDR fields\n");
    fprintf(header, "    int mdb_type;\n");
    fprintf(header, "    int address_qualifier;\n");
    fprintf(header, "    uint32_t address;\n");
    fprintf(header, "    \n");
    fprintf(header, "    // Decoded SV fields\n");
    fprintf(header, "    int has_sv;\n");
    fprintf(header, "    int nic;\n");
    fprintf(header, "    int position_valid;\n");
    fprintf(header, "    double lat;\n");
    fprintf(header, "    double lon;\n");
    fprintf(header, "    int altitude_type;\n");
    fprintf(header, "    int altitude;\n");
    fprintf(header, "    int airground_state;\n");
    fprintf(header, "    int ns_vel_valid;\n");
    fprintf(header, "    int ns_vel;\n");
    fprintf(header, "    int ew_vel_valid;\n");
    fprintf(header, "    int ew_vel;\n");
    fprintf(header, "    int track_type;\n");
    fprintf(header, "    uint16_t track;\n");
    fprintf(header, "    int speed_valid;\n");
    fprintf(header, "    uint16_t speed;\n");
    fprintf(header, "    int vert_rate_source;\n");
    fprintf(header, "    int vert_rate;\n");
    fprintf(header, "    int dimensions_valid;\n");
    fprintf(header, "    double length;\n");
    fprintf(header, "    double width;\n");
    fprintf(header, "    int position_offset;\n");
    fprintf(header, "    int utc_coupled;\n");
    fprintf(header, "    int tisb_site_id;\n");
    fprintf(header, "    \n");
    fprintf(header, "    // Decoded MS fields\n");
    fprintf(header, "    int has_ms;\n");
    fprintf(header, "    int emitter_category;\n");
    fprintf(header, "    char callsign[9];\n");
    fprintf(header, "    int callsign_type;\n");
    fprintf(header, "    int emergency_status;\n");
    fprintf(header, "    int uat_version;\n");
    fprintf(header, "    int sil;\n");
    fprintf(header, "    int transmit_mso;\n");
    fprintf(header, "    int nac_p;\n");
    fprintf(header, "    int nac_v;\n");
    fprintf(header, "    int nic_baro;\n");
    fprintf(header, "    int has_cdti;\n");
    fprintf(header, "    int has_acas;\n");
    fprintf(header, "    int acas_ra_active;\n");
    fprintf(header, "    int ident_active;\n");
    fprintf(header, "    int atc_services;\n");
    fprintf(header, "    int heading_type;\n");
    fprintf(header, "    \n");
    fprintf(header, "    // Decoded AUXSV fields\n");
    fprintf(header, "    int has_auxsv;\n");
    fprintf(header, "    int sec_altitude_type;\n");
    fprintf(header, "    int sec_altitude;\n");
    fprintf(header, "} uat_downlink_test_frame_t;\n\n");

    fprintf(header, "// Uplink test frame structure\n");
    fprintf(header, "typedef struct {\n");
    fprintf(header, "    const char* frame_data_hex;\n");
    fprintf(header, "    int frame_length;\n");
    fprintf(header, "    const char* test_name;\n");
    fprintf(header, "    \n");
    fprintf(header, "    // Decoded uplink fields\n");
    fprintf(header, "    int position_valid;\n");
    fprintf(header, "    double lat;\n");
    fprintf(header, "    double lon;\n");
    fprintf(header, "    int utc_coupled;\n");
    fprintf(header, "    int app_data_valid;\n");
    fprintf(header, "    int slot_id;\n");
    fprintf(header, "    int tisb_site_id;\n");
    fprintf(header, "    int num_info_frames;\n");
    fprintf(header, "    \n");
    fprintf(header, "    // Info frame data\n");
    fprintf(header, "    struct {\n");
    fprintf(header, "        int length;\n");
    fprintf(header, "        int type;\n");
    fprintf(header, "        int is_fisb;\n");
    fprintf(header, "        int fisb_product_id;\n");
    fprintf(header, "        int fisb_a_flag;\n");
    fprintf(header, "        int fisb_g_flag;\n");
    fprintf(header, "        int fisb_p_flag;\n");
    fprintf(header, "        int fisb_s_flag;\n");
    fprintf(header, "        int fisb_hours;\n");
    fprintf(header, "        int fisb_minutes;\n");
    fprintf(header, "        int fisb_seconds;\n");
    fprintf(header, "        int fisb_seconds_valid;\n");
    fprintf(header, "        int fisb_month;\n");
    fprintf(header, "        int fisb_day;\n");
    fprintf(header, "        int fisb_monthday_valid;\n");
    fprintf(header, "    } info_frames[8];\n");
    fprintf(header, "} uat_uplink_test_frame_t;\n\n");

    fprintf(header, "#ifdef __cplusplus\n");
    fprintf(header, "extern \"C\" {\n");
    fprintf(header, "#endif\n\n");

    fprintf(header, "// Function declarations\n");
    fprintf(header, "const uat_downlink_test_frame_t* get_uat_downlink_test_frames();\n");
    fprintf(header, "int get_uat_downlink_test_frames_count();\n");
    fprintf(header, "const uat_downlink_test_frame_t* get_uat_downlink_test_frame(int index);\n\n");

    fprintf(header, "const uat_uplink_test_frame_t* get_uat_uplink_test_frames();\n");
    fprintf(header, "int get_uat_uplink_test_frames_count();\n");
    fprintf(header, "const uat_uplink_test_frame_t* get_uat_uplink_test_frame(int index);\n\n");

    fprintf(header, "#ifdef __cplusplus\n");
    fprintf(header, "}\n");
    fprintf(header, "#endif\n\n");
    fprintf(header, "#endif // UAT_TEST_DATA_H\n");

    fclose(header);
    printf("Generated header file: %s\n", header_filename);
}

int main(int argc, char **argv)
{
    struct dump978_reader *reader;
    int framecount;
    const char *output_dir = "scripts/"; // Default to current directory
    char output_filename[512];
    char header_filename[512];
    FILE *output;

    // Construct full file paths
    snprintf(output_filename, sizeof(output_filename), "%s/uat_test_data.c", output_dir);
    snprintf(header_filename, sizeof(header_filename), "%s/uat_test_data.h", output_dir);

    printf("Output directory: %s\n", output_dir);
    printf("Will generate:\n");
    printf("  %s\n", output_filename);
    printf("  %s\n", header_filename);

    reader = dump978_reader_new(0, 0);
    if (!reader)
    {
        perror("dump978_reader_new");
        return 1;
    }

    // Read all frames
    while ((framecount = dump978_read_frames(reader, handle_frame, NULL)) > 0)
        ;

    if (framecount < 0)
    {
        perror("dump978_read_frames");
        return 1;
    }

    // Output the test data
    output = fopen(output_filename, "w");
    if (!output)
    {
        fprintf(stderr, "Failed to create output file: %s\n", output_filename);
        perror("fopen");
        return 1;
    }

    output_test_header(output);
    output_downlink_data(output);
    output_uplink_data(output);
    output_accessor_functions(output);

    fclose(output);

    // Generate header file
    output_header_file(header_filename);

    printf("Generated %d downlink frames and %d uplink frames in %s\n",
           num_downlink_frames, num_uplink_frames, output_filename);
    printf("\nUsage: %s [output_directory]\n", argv[0]);
    printf("  output_directory: Directory where uat_test_data.c and uat_test_data.h will be created (default: current directory)\n\n");
    printf("Usage in Google Test:\n");
    printf("  #include \"uat_test_data.h\"\n\n");
    printf("  // Test downlink frames\n");
    printf("  TEST(UATDecoderTest, DownlinkFrames) {\n");
    printf("      int count = get_uat_downlink_test_frames_count();\n");
    printf("      for (int i = 0; i < count; i++) {\n");
    printf("          const uat_downlink_test_frame_t* frame = get_uat_downlink_test_frame(i);\n");
    printf("          SCOPED_TRACE(frame->test_name);\n");
    printf("          \n");
    printf("          // Convert hex string back to bytes for your decoder\n");
    printf("          uint8_t frame_bytes[48];\n");
    printf("          hex_string_to_bytes(frame->frame_data_hex, frame_bytes, frame->frame_length);\n");
    printf("          \n");
    printf("          // Decode with your decoder\n");
    printf("          YourResult result = your_decoder.decode_downlink(frame_bytes, frame->frame_length);\n");
    printf("          \n");
    printf("          // Compare against expected values\n");
    printf("          EXPECT_EQ(result.mdb_type, frame->mdb_type);\n");
    printf("          EXPECT_EQ(result.address, frame->address);\n");
    printf("          if (frame->position_valid) {\n");
    printf("              EXPECT_NEAR(result.lat, frame->lat, 0.0001);\n");
    printf("              EXPECT_NEAR(result.lon, frame->lon, 0.0001);\n");
    printf("          }\n");
    printf("          // ... more assertions\n");
    printf("      }\n");
    printf("  }\n\n");
    printf("  // Helper function to convert hex string to bytes:\n");
    printf("  void hex_string_to_bytes(const char* hex_string, uint8_t* bytes, int length) {\n");
    printf("      for (int i = 0; i < length; i++) {\n");
    printf("          sscanf(hex_string + (i * 2), \"%%2hhx\", &bytes[i]);\n");
    printf("      }\n");
    printf("  }\n");

    return 0;
}