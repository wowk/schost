#include "debug.h"
#include <util.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/ether.h>
#include <host_gecko.h>


#define swap(a, b) do{\
    typeof(a) tmp = b;\
    b = a;\
    a = tmp;\
}while(0)

uint16_t to_uuid16(uint8array* uuid)
{
	uint16_t val;
    if(uuid->len == 16){
	    val = (uuid->data[2] << 8) + uuid->data[3];
    }else{
        val = (uuid->data[0]<<8) + uuid->data[1];
    }

	return val;
}

size_t hex2str(uint8_t* data, size_t len, char* buffer)
{
    //strcpy(buffer, "0x");
    //buffer += 2;
    for(int i = 0 ; i < len ; i ++){
        sprintf(buffer, "%.2X ", data[i]);
        buffer += 2;
    }

    return len;
}

char* btaddr2str(void* addr,  char* buf)
{
    uint8_t* p =(uint8_t*)addr;
    const char* fmt = "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x";

    sprintf(buf, fmt, p[5], p[4], p[3], p[2], p[1], p[0]);

    return buf;
}

void* str2btaddr(char* str, void* buf)
{
    uint8_t* p = (uint8_t*)buf;

    ether_aton_r(str, (struct ether_addr*)buf);
    swap(p[5], p[0]);
    swap(p[4], p[1]);
    swap(p[3], p[2]);
    
    return buf;
}

void echo(int append, const char* file, const char* format, ...)
{
    if (append && access(file, R_OK) < 0) {
        append = 0;
    }
    FILE* fp = fopen(file, append ? "a" : "w");
    if (!fp) {
        error(0, errno, "failed to open file <%s>", file);
        return;
    }
    va_list val;
    va_start(val, format);
    vfprintf(fp, format, val);
    va_end(val);
    fclose(fp);
}

size_t cat(const char* file, char** pdata, size_t len)
{
    *pdata = NULL;
    FILE* fp = fopen(file, "r");
    if (!fp) {
        error(0, errno, "failed to open file <%s>", file);
        goto error;
    }
    if (len == 0) {
        fseek(fp, 0, SEEK_END);
        len = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        if (len < 0) {
            error(0, errno, "failed to get file <%s> size", file);
            goto error;
        }
    }
    *pdata = (char*)Calloc(1, len+1);
    if (!(*pdata)) {
        error(0, errno, "failed to malloc");
        goto error;
    }
    len = fread(*pdata, 1, len, fp);
    fclose(fp);

    return len;
error:
    if (fp)
        fclose(fp);
    return -1;
}

void show_file(FILE* fp, const char* file, size_t len)
{
    char* pdata;
    cat(file, &pdata, len);
    if (pdata) {
        fprintf(fp, "%s\n", pdata);
        free(pdata);
    }
}

struct ble_error_t {
    uint16_t err_code;
    const char* err_name;
    const char* err_msg;
};

const struct ble_error_t ble_errors[] = {
    {0x0501, "ps_store_full", "Flash reserved for PS store is full"},
    {0x0502, "ps_key_not_found", "PS key not found"},
    {0x0503, "i2c_ack_missing", "Acknowledge for i2c was not received."},
    {0x0504, "i2c_timeout", "I2C read or write timed out."},
    {0x0101, "invalid_conn_handle", "Invalid GATT connection handle."},
    {0x0102, "waiting_response", "Waiting response from GATT server to previous procedure."},
    {0x0103, "gatt_connection_timeout", "GATT connection is closed due procedure timeout."},
    {0x0180, "invalid_param", "Command contained invalid parameter"},
    {0x0181, "wrong_state", "Device is in wrong state to receive command"},
    {0x0182, "out_of_memory", "Device has run out of memory"},
    {0x0183, "not_implemented", "Feature is not implemented"},
    {0x0184, "invalid_command", "Command was not recognized"},
    {0x0185, "timeout", "A command or procedure failed or a link lost due to timeout"},
    {0x0186, "not_connected", "Connection handle passed is to command is not a valid handle"},
    {0x0187, "flow", "Command would cause either underflow or overflow error"},
    {0x0188, "user_attribute", "User attribute was accessed through API which is not supported"},
    {0x0189, "invalid_license_key", "No valid license key found"},
    {0x018a, "command_too_long", "Command maximum length exceeded"},
    {0x018b, "out_of_bonds", "Bonding procedure can't be started because device has no space left for bond."},
    {0x018c, "unspecified", "Unspecified error"},
    {0x018d, "hardware", "Hardware failure"},
    {0x018e, "buffers_full", "Command not accepted, because internal buffers are full"},
    {0x018f, "disconnected", "Command or Procedure failed due to disconnection"},
    {0x0190, "too_many_requests", "Too many Simultaneous Requests"},
    {0x0191, "not_supported", "Feature is not supported in this firmware build"},
    {0x0192, "no_bonding", "The bonding does not exist."},
    {0x0193, "crypto", "Error using crypto functions"},
    {0x0194, "data_corrupted", "Data was corrupted."},
    {0x0195, "command_incomplete", "Data received does not form a complete command"},
    {0x0196, "not_initialized", "Feature or subsystem not initialized"},
    {0x0197, "invalid_sync_handle", "Invalid periodic advertising sync handle"},
    {0x0198, "invalid_module_action", "Bluetooth cannot be used on this hardware"},
    {0x0199, "radio", "Error received from radio"},
    {0x0301, "passkey_entry_failed", "The user input of passkey failed, for example, the user cancelled the operation"},
    {0x0302, "oob_not_available", "Out of Band data is not available for authentication"},
    {0x0303, "authentication_requirements", "The pairing procedure cannot be performed as authentication requirements cannot be met due to IO capabilities of one or both devices"},
    {0x0304, "confirm_value_failed", "The confirm value does not match the calculated compare value"},
    {0x0305, "pairing_not_supported", "Pairing is not supported by the device"},
    {0x0306, "encryption_key_size", "The resultant encryption key size is insufficient for the security requirements of this device"},
    {0x0307, "command_not_supported", "The SMP command received is not supported on this device"},
    {0x0308, "unspecified_reason", "Pairing failed due to an unspecified reason"},
    {0x0309, "repeated_attempts", "Pairing or authentication procedure is disallowed because too little time has elapsed since last pairing request or security request"},
    {0x030a, "invalid_parameters", "The Invalid Parameters error code indicates: the command length is invalid or a parameter is outside of the specified range."},
    {0x030b, "dhkey_check_failed", "Indicates to the remote device that the DHKey Check value received doesn't match the one calculated by the local device."},
    {0x030c, "numeric_comparison_failed", "Indicates that the confirm values in the numeric comparison protocol do not match."},
    {0x030d, "bredr_pairing_in_progress", "Indicates that the pairing over the LE transport failed due to a Pairing Request sent over the BR/EDR transport in process."},
    {0x030e, "cross_transport_key_derivation_generation_not_allowed", "Indicates that the BR/EDR Link Key generated on the BR/EDR transport cannot be used to derive and distribute keys for the LE transport."},
    {0x0202, "unknown_connection_identifier", "Connection does not exist, or connection open request was cancelled."},
    {0x0205, "authentication_failure", "Pairing or authentication failed due to incorrect results in the pairing or authentication procedure. This could be due to an incorrect PIN or Link Key"},
    {0x0206, "pin_or_key_missing", "Pairing failed because of missing PIN, or authentication failed because of missing Key"},
    {0x0207, "memory_capacity_exceeded", "Controller is out of memory."},
    {0x0208, "connection_timeout", "Link supervision timeout has expired."},
    {0x0209, "connection_limit_exceeded", "Controller is at limit of connections it can support."},
    {0x020a, "synchronous_connectiontion_limit_exceeded", "The Synchronous Connection Limit to a Device Exceeded error code indicates that the Controller has reached the limit to the number of synchronous connections that can be achieved to a device."},
    {0x020b, "acl_connection_already_exists", "The ACL Connection Already Exists error code indicates that an attempt to create a new ACL Connection to a device when there is already a connection to this device."},
    {0x020c, "command_disallowed", "Command requested cannot be executed because the Controller is in a state where it cannot process this command at this time."},
    {0x020d, "connection_rejected_due_to_limited_resources", "The Connection Rejected Due To Limited Resources error code indicates that an incoming connection was rejected due to limited resources."},
    {0x020e, "connection_rejected_due_to_security_reasons", "The Connection Rejected Due To Security Reasons error code indicates that a connection was rejected due to security requirements not being fulfilled, like authentication or pairing."},
    {0x020f, "connection_rejected_due_to_unacceptable_bd_addr", "The Connection was rejected because this device does not accept the BD_ADDR. This may be because the device will only accept connections from specific BD_ADDRs."},
    {0x0210, "connection_accept_timeout_exceeded", "The Connection Accept Timeout has been exceeded for this connection attempt."},
    {0x0211, "unsupported_feature_or_parameter_value", "A feature or parameter value in the HCI command is not supported."},
    {0x0212, "invalid_command_parameters", "Command contained invalid parameters."},
    {0x0213, "remote_user_terminated", "User on the remote device terminated the connection."},
    {0x0214, "remote_device_terminated_connection_due_to_low_resources", "The remote device terminated the connection because of low resources"},
    {0x0215, "remote_powering_off", "Remote Device Terminated Connection due to Power Off"},
    {0x0216, "connection_terminated_by_local_host", "Local device terminated the connection."},
    {0x0217, "repeated_attempts", "The Controller is disallowing an authentication or pairing procedure because too little time has elapsed since the last authentication or pairing attempt failed."},
    {0x0218, "pairing_not_allowed", "The device does not allow pairing. This can be for example, when a device only allows pairing during a certain time window after some user input allows pairing"},
    {0x021a, "unsupported_remote_feature", "The remote device does not support the feature associated with the issued command."},
    {0x021f, "unspecified_error", "No other error code specified is appropriate to use."},
    {0x0222, "ll_response_timeout", "Connection terminated due to link-layer procedure timeout."},
    {0x0223, "ll_procedure_collision", "LL procedure has collided with the same transaction or procedure that is already in progress."},
    {0x0225, "encryption_mode_not_acceptable", "The requested encryption mode is not acceptable at this time."},
    {0x0226, "link_key_cannot_be_changed", "Link key cannot be changed because a fixed unit key is being used."},
    {0x0228, "instant_passed", "LMP PDU or LL PDU that includes an instant cannot be performed because the instant when this would have occurred has passed."},
    {0x0229, "pairing_with_unit_key_not_supported", "It was not possible to pair as a unit key was requested and it is not supported."},
    {0x022a, "different_transaction_collision", "LMP transaction was started that collides with an ongoing transaction."},
    {0x022e, "channel_assessment_not_supported", "The Controller cannot perform channel assessment because it is not supported."},
    {0x022f, "insufficient_security", "The HCI command or LMP PDU sent is only possible on an encrypted link."},
    {0x0230, "parameter_out_of_mandatory_range", "A parameter value requested is outside the mandatory range of parameters for the given HCI command or LMP PDU."},
    {0x0237, "simple_pairing_not_supported_by_host", "The IO capabilities request or response was rejected because the sending Host does not support Secure Simple Pairing even though the receiving Link Manager does."},
    {0x0238, "host_busy_pairing", "The Host is busy with another pairing operation and unable to support the requested pairing. The receiving device should retry pairing again later."},
    {0x0239, "connection_rejected_due_to_no_suitable_channel_found", "The Controller could not calculate an appropriate value for the Channel selection operation."},
    {0x023a, "controller_busy", "Operation was rejected because the controller is busy and unable to process the request."},
    {0x023b, "unacceptable_connection_interval", "Remote device terminated the connection because of an unacceptable connection interval."},
    {0x023c, "advertising_timeout", "Ddvertising for a fixed duration completed or, for directed advertising, that advertising completed without a connection being created."},
    {0x023d, "connection_terminated_due_to_mic_failure", "Connection was terminated because the Message Integrity Check (MIC) failed on a received packet."},
    {0x023e, "connection_failed_to_be_established", "LL initiated a connection but the connection has failed to be established. Controller did not receive any packets from remote end."},
    {0x023f, "mac_connection_failed", "The MAC of the 802.11 AMP was requested to connect to a peer, but the connection failed."},
    {0x0240, "coarse_clock_adjustment_rejected_but_will_try_to_adjust_using_clock_dragging", "The master, at this time, is unable to make a coarse adjustment to the piconet clock, using the supplied parameters. Instead the master will attempt to move the clock using clock dragging."},
    {0x0242, "unknown_advertising_identifier", "A command was sent from the Host that should identify an Advertising or Sync handle, but the Advertising or Sync handle does not exist."},
    {0x0243, "limit_reached", "Number of operations requested has been reached and has indicated the completion of the activity (e.g., advertising or scanning)."},
    {0x0244, "operation_cancelled_by_host", "A request to the Controller issued by the Host and still pending was successfully canceled."},
    {0x0245, "packet_too_long", "An attempt was made to send or receive a packet that exceeds the maximum allowed packet length."},
    {0x0a01, "file_open_failed", "File open failed."},
    {0x0a02, "xml_parse_failed", "XML parsing failed."},
    {0x0a03, "device_connection_failed", "Device connection failed."},
    {0x0a04, "device_comunication_failed", "Device communication failed."},
    {0x0a05, "authentication_failed", "Device authentication failed."},
    {0x0a06, "incorrect_gatt_database", "Device has incorrect GATT database."},
    {0x0a07, "disconnected_due_to_procedure_collision", "Device disconnected due to procedure collision."},
    {0x0a08, "disconnected_due_to_secure_session_failed", "Device disconnected due to failure to establish or reestablish a secure session."},
    {0x0a09, "encryption_decryption_error", "Encrypion/decryption operation failed."},
    {0x0a0a, "maximum_retries", "Maximum allowed retries exceeded."},
    {0x0a0b, "data_parse_failed", "Data parsing failed."},
    {0x0a0c, "pairing_removed", "Pairing established by the application layer protocol has been removed."},
    {0x0a0d, "inactive_timeout", "Inactive timeout."},
    {0x0a0e, "mismatched_or_insufficient_security", "Mismatched or insufficient security level"},
    {0x0401, "invalid_handle", "The attribute handle given was not valid on this server"},
    {0x0402, "read_not_permitted", "The attribute cannot be read"},
    {0x0403, "write_not_permitted", "The attribute cannot be written"},
    {0x0404, "invalid_pdu", "The attribute PDU was invalid"},
    {0x0405, "insufficient_authentication", "The attribute requires authentication before it can be read or written."},
    {0x0406, "request_not_supported", "Attribute Server does not support the request received from the client."},
    {0x0407, "invalid_offset", "Offset specified was past the end of the attribute"},
    {0x0408, "insufficient_authorization", "The attribute requires authorization before it can be read or written."},
    {0x0409, "prepare_queue_full", "Too many prepare writes have been queueud"},
    {0x040a, "att_not_found", "No attribute found within the given attribute handle range."},
    {0x040b, "att_not_long", "The attribute cannot be read or written using the Read Blob Request"},
    {0x040c, "insufficient_enc_key_size", "The Encryption Key Size used for encrypting this link is insufficient."},
    {0x040d, "invalid_att_length", "The attribute value length is invalid for the operation"},
    {0x040e, "unlikely_error", "The attribute request that was requested has encountered an error that was unlikely, and therefore could not be completed as requested."},
    {0x040f, "insufficient_encryption", "The attribute requires encryption before it can be read or written."},
    {0x0410, "unsupported_group_type", "The attribute type is not a supported grouping attribute as defined by a higher layer specification."},
    {0x0411, "insufficient_resources", "Insufficient Resources to complete the request"},
    {0x0412, "out_of_sync", "The server requests the client to rediscover the database."},
    {0x0413, "value_not_allowed", "The attribute parameter value was not allowed."},
    {0x0480, "application", "When this is returned in a BGAPI response, the application tried to read or write the value of a user attribute from the GATT database."},
    {0x0c01, "already_exists", "Returned when trying to add a key or some other unique resource with an ID which already exists"},
    {0x0c02, "does_not_exist", "Returned when trying to manipulate a key or some other resource with an ID which does not exist"},
    {0x0c03, "limit_reached", "Returned when an operation cannot be executed because a pre-configured limit for keys, key bindings, elements, models, virtual addresses, provisioned devices, or provisioning sessions is reached"},
    {0x0c04, "invalid_address", "Returned when trying to use a reserved address or add a \"pre-provisioned\" device using an address already used by some other device"},
    {0x0c05, "malformed_data", "In a BGAPI response, the user supplied malformed data; in a BGAPI event, the remote end responded with malformed or unrecognized data"},
    {0x0c06, "already_initialized", "An attempt was made to initialize a subsystem that was already initialized."},
    {0x0c07, "not_initialized", "An attempt was made to use a subsystem that wasn't initialized yet. Call the subsystem's init function first."},
    {0x0c08, "no_friend_offer", "Returned when trying to establish a friendship as a Low Power Node, but no acceptable friend offer message was received."},
    {0x0c09, "prov_link_closed", "Provisioning link was unexpectedly closed before provisioning was complete."},
    {0x0c0a, "prov_invalid_pdu", "An unrecognized provisioning PDU was received."},
    {0x0c0b, "prov_invalid_pdu_format", "A provisioning PDU with wrong length or containing field values that are out of bounds was received."},
    {0x0c0c, "prov_unexpected_pdu", "An unexpected (out of sequence) provisioning PDU was received."},
    {0x0c0d, "prov_confirmation_failed", "The computed confirmation value did not match the expected value."},
    {0x0c0e, "prov_out_of_resources", "Provisioning could not be continued due to insufficient resources."},
    {0x0c0f, "prov_decryption_failed", "The provisioning data block could not be decrypted."},
    {0x0c10, "prov_unexpected_error", "An unexpected error happened during provisioning."},
    {0x0c11, "prov_cannot_assign_addr", "Device could not assign unicast addresses to all of its elements."},
    {0x0c12, "address_temporarily_unavailable", "Returned when trying to reuse an address of a previously deleted device before an IV Index Update has been executed."},
    {0x0c13, "address_already_used", "Returned when trying to assign an address that is used by one of the devices in the Device Database, or by the Provisioner itself."},
    {0x0e01, "invalid_address", "Returned when address in request was not valid"},
    {0x0e02, "invalid_model", "Returned when model identified is not found for a given element"},
    {0x0e03, "invalid_app_key", "Returned when the key identified by AppKeyIndex is not stored in the node"},
    {0x0e04, "invalid_net_key", "Returned when the key identified by NetKeyIndex is not stored in the node"},
    {0x0e05, "insufficient_resources", "Returned when The node cannot serve the request due to insufficient resources"},
    {0x0e06, "key_index_exists", "Returned when the key identified is already stored in the node and the new NetKey value is different"},
    {0x0e07, "invalid_publish_params", "Returned when the model does not support the publish mechanism"},
    {0x0e08, "not_subscribe_model", "Returned when the model does not support the subscribe mechanism"},
    {0x0e09, "storage_failure", "Returned when storing of the requested parameters failed"},
    {0x0e0a, "not_supported", "Returned when requested setting is not supported"},
    {0x0e0b, "cannot_update", "Returned when the requested update operation cannot be performed due to general constraints"},
    {0x0e0c, "cannot_remove", "Returned when the requested delete operation cannot be performed due to general constraints"},
    {0x0e0d, "cannot_bind", "Returned when the requested bind operation cannot be performed due to general constraints"},
    {0x0e0e, "temporarily_unable", "Returned when The node cannot start advertising with Node Identity or Proxy since the maximum number of parallel advertising is reached"},
    {0x0e0f, "cannot_set", "Returned when the requested state cannot be set"},
    {0x0e10, "unspecified", "Returned when an unspecified error took place"},
    {0x0e11, "invalid_binding", "Returned when the NetKeyIndex and AppKeyIndex combination is not valid for a Config AppKey Update"},
    {0x0901, "file_not_found", "File not found"},
    {0x0d01, "remote_disconnected", "Returned when remote disconnects the connection-oriented channel by sending disconnection request."},
    {0x0d02, "local_disconnected", "Returned when local host disconnect the connection-oriented channel by sending disconnection request."},
    {0x0d03, "cid_not_exist", "Returned when local host did not find a connection-oriented channel with given destination CID."},
    {0x0d04, "le_disconnected", "Returned when connection-oriented channel disconnected due to LE connection is dropped."},
    {0x0d05, "flow_control_violated", "Returned when connection-oriented channel disconnected due to remote end send data even without credit."},
    {0x0d06, "flow_control_credit_overflowed", "Returned when connection-oriented channel disconnected due to remote end send flow control credits exceed 65535."},
    {0x0d07, "no_flow_control_credit", "Returned when connection-oriented channel has run out of flow control credit and local application still trying to send data."},
    {0x0d08, "connection_request_timeout", "Returned when connection-oriented channel has not received connection response message within maximum timeout."},
    {0x0d09, "invalid_cid", "Returned when local host received a connection-oriented channel connection response with an invalid destination CID."},
    {0x0d0a, "wrong_state", "Returned when local host application tries to send a command which is not suitable for L2CAP channel's current state."},
    {0x0b01, "image_signature_verification_failed", "Device firmware signature verification failed."},
    {0x0b02, "file_signature_verification_failed", "File signature verification failed."},
    {0x0b03, "image_checksum_error", "Device firmware checksum is not valid."}
};


const char* error_summary(int result)
{
    const char* errptr = "";

    for(int i = 0 ; i < sizeof(ble_errors)/sizeof(ble_errors[0]) ; i ++){
        if(result == ble_errors[i].err_code){
            return ble_errors[i].err_msg;
            break;
        }
    }

    return errptr;
}

int process_running(const char* pidfile)
{
    if(access(pidfile, F_OK) < 0){
        return 0;
    }
    
    char* pid = NULL;
    cat(pidfile, &pid, sizeof(pid));
    if(!pid){
        return 1;
    }

    char cmdline[64] = "";
    sprintf(cmdline, "/proc/%s/cmdline", pid);
    free(pid);

    if(access(cmdline, F_OK) < 0){
        echo(0, pidfile, "%d", (int)getpid());
        return 0;
    }
    
    return 1;
}
