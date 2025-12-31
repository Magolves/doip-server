import udsoncan
from doipclient import DoIPClient
from doipclient.connectors import DoIPClientUDSConnector
from udsoncan.client import Client
from udsoncan.exceptions import *
from udsoncan.services import *
from udsoncan import DataIdentifier, AsciiCodec, MemoryLocation
from udsoncan.services import DiagnosticSessionControl
from time import sleep

udsoncan.setup_logging()

# Add this config
config = {
    'data_identifiers': {
        DataIdentifier.VIN: AsciiCodec(17)
    }
}

ecu_ip = '127.0.0.1'
ecu_logical_address = 0x00E0
doip_client = DoIPClient(ecu_ip, ecu_logical_address)
conn = DoIPClientUDSConnector(doip_client)
with Client(conn, request_timeout=2, config=config) as client:
   try:
      # Switch to programming session using symbolic constant
      client.change_session(DiagnosticSessionControl.Session.programmingSession)

      # Read VIN once and handle response type (udsoncan may return bytes or str)
      vin_response = client.read_data_by_identifier(udsoncan.DataIdentifier.VIN)
      vin_value = vin_response.service_data.values[udsoncan.DataIdentifier.VIN]
      if isinstance(vin_value, bytes):
         vin_str = vin_value.decode('ascii', errors='ignore')
      else:
         # already a str
         vin_str = str(vin_value)
      print('Current Vehicle Identification Number is: %s' % vin_str)

      client.write_data_by_identifier(udsoncan.DataIdentifier.VIN, 'ABC123456789GHJKL')       # Standard ID for VIN is 0xF190. Codec is set in the client configuration
      print('Vehicle Identification Number successfully changed.')

      # ====== Download Image Example ======
      # This demonstrates using UDS services 0x34 (RequestDownload), 0x36 (TransferData), and 0x37 (RequestTransferExit)
      print('\n--- Starting firmware download example ---')

      # Example firmware data to download
      firmware_data = b'\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09' * 10  # 100 bytes of test data
      memory_address = 0x40000000  # Target memory address (example)
      memory_size = len(firmware_data)

      # Step 1: RequestDownload (0x34) - Initiate the download transfer
      memory_location = MemoryLocation(address=memory_address, memorysize=memory_size)
      print('Requesting download to address 0x%08X, size %d bytes...' % (memory_address, memory_size))
      download_response = client.request_download(memory_location)
      max_block_length = download_response.service_data.max_length
      print('Download accepted. Max block length: %d bytes' % max_block_length)

      # Step 2: TransferData (0x36) - Transfer the actual data in blocks
      print('Transferring data...')
      block_sequence_counter = 1
      offset = 0
      while offset < len(firmware_data):
         # Calculate block size (respect max_block_length from server)
         block_size = min(max_block_length, len(firmware_data) - offset)
         block_data = firmware_data[offset:offset + block_size]

         # Send the data block
         client.transfer_data(block_sequence_counter, block_data)
         print('  Block %d: %d bytes transferred' % (block_sequence_counter, block_size))

         offset += block_size
         block_sequence_counter = (block_sequence_counter + 1) & 0xFF  # Keep counter in 0-255 range

      # Step 3: RequestTransferExit (0x37) - Finalize the download
      print('Requesting transfer exit...')
      client.request_transfer_exit()
      print('Download completed successfully!')
      print('--- Firmware download example finished ---\n')

      sleep(1)  # wait a bit
      # ping to keep session alive
      client.tester_present()
      # close diag session - return to default session
      client.change_session(DiagnosticSessionControl.Session.defaultSession)
   except NegativeResponseException as e:
      print('Server refused our request for service %s with code "%s" (0x%02x)' % (e.response.service.get_name(), e.response.code_name, e.response.code))
      raise  # Re-raise to exit with error
   except (InvalidResponseException, UnexpectedResponseException) as e:
      print('Server sent an invalid payload : %s' % e.response.original_payload)
      raise  # Re-raise to exit with error
   finally:
      # Cleanup the DoIP Socket when we're done. Alternatively, we could have used the
      # close_connection flag on conn so that the udsoncan client would clean it up
      doip_client.close()