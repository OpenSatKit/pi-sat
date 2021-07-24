import sys
import time
import configparser
import socket
import threading
import struct
import subprocess
import os.path

#
# Provides a UDP connection that manages a simple pisat command & telemetry control
# interface for managing OSK's Rasberry Pi cfS target. pictrl.ini defines configuration
# parameters that are read during initialization. 
#
# See OSK's pi-sat Quick Start for instructions on how to automatically run this program
# when the Pi starts
#
class PiCtrl:

   def __init__(self, IniFile):

      self.config = configparser.ConfigParser()
      self.config.read(IniFile)
      self.exec = self.config['exec']
      self.network = self.config['network']   
      self.cfs = self.config['cfs']
      
   
      self.commands = { 'CFS_START': self.start_cfs(self), 'CFS_STOP': self.stop_cfs(self) }
      self.cmd_valid_cnt = 0
      self.cmd_invalid_cnt = 0
      
      self.udp_server_socket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
      self.udp_server_socket.bind((self.network['cmd-ip-addr'], self.config.getint('network','cmd-port')))
      
      self.cfs_running = False
      self.cfs_process = None
      self.cfs_app_str = ""
      self.cfs_app_tlm = bytes(128)
      
   def execute(self):
     
      tlm_client = self.Telemetry(self, (self.network['tlm-ip-addr'], self.config.getint('network','tlm-port')), 
                                  self.config.getint('exec','tlm-delay'), self.exec['tlm-pkt-id'])
      tlm_thread = threading.Thread(target=tlm_client.send)
      tlm_thread.start()
    
      while True:
         try:
            
            msg_addr_array = self.udp_server_socket.recvfrom(self.config.getint('network','cmd-buf-size'))
            message = msg_addr_array[0]
            address = msg_addr_array[1]

            cmd_str = message.decode()
            
            if cmd_str in self.commands:
               if (self.commands[cmd_str].execute()):
                  self.cmd_valid_cnt += 1
               else:
                  self.cmd_invalid_cnt += 1
            else:
               print("Received undefined command %s" % cmd_str)
               self.cmd_invalid_cnt += 1
                              
         except KeyboardInterrupt:
            print('Intercepted Control-C, exiting')
            tlm_client.terminate()
            sys.exit() 

   def create_cfs_app_str(self, target_path):

      startup_path_file = os.path.join(target_path, 'cf', 'cfe_es_startup.scr')
      with open(startup_path_file) as f:
         startup_file = f.readlines()
      
      self.cfs_app_str = ""
      first_app = True
      for startup_line in startup_file:
         line = startup_line.split(',')
         if '!' in line[0]:
            break
         else:
            if first_app:
               self.cfs_app_str += line[3].strip()
               first_app = False
            else:
               self.cfs_app_str += ', ' + line[3].strip()
            
      self.cfs_app_tlm = bytes(self.cfs_app_str.ljust(128, '\0'), 'utf-8')


   def clear_cfs_app_str(self):
      
      self.cfs_app_str = ""   
      self.cfs_app_tlm = bytes(self.cfs_app_str.ljust(128, '\0'), 'utf-8')


   #
   # Command Inner Classes
   #
   class Command:
      def __init__(self, pi_ctrl):
         self.pi_ctrl = pi_ctrl
      def virtual_execute(self):
         raise NotImplementedError()
      def execute(self):
         return self.virtual_execute()
         
   class start_cfs(Command):
      
      def virtual_execute(self):
   
         ret_status = False
         
         if pi_ctrl.cfs_running:
            print("Start cFS command rejected. The cFS is already running, pid = %d." % pi_ctrl.cfs_process.pid)
         else:
            pi_ctrl.create_cfs_app_str(pi_ctrl.cfs['target-path'])            
            pi_ctrl.cfs_process = subprocess.Popen([pi_ctrl.cfs['target-binary']], cwd=pi_ctrl.cfs['target-path'], shell=False)
            pi_ctrl.cfs_running = True
            ret_status = True
            print("Start cFS, pid = %d" % pi_ctrl.cfs_process.pid)
      
         return ret_status
         

   class stop_cfs(Command):
      
      def virtual_execute(self):
      
         ret_status = False

         if pi_ctrl.cfs_running:
            pi_ctrl.clear_cfs_app_str()
            print("Stopping cFS, pid = %d" % pi_ctrl.cfs_process.pid)
            subprocess.call(["kill", "-9", "%d" % pi_ctrl.cfs_process.pid])
            pi_ctrl.cfs_process.wait()
            pi_ctrl.cfs_running = False
            ret_status = True
         else:
            print("Stop cFS command rejected. The cFS is not running.")
         
         return ret_status

   #
   # Telemetry Inner Class
   #
   class Telemetry:
      def __init__(self, pi_ctrl, server_addr_port, send_delay, pkt_id_hex_str):
      
         self.pi_ctrl = pi_ctrl     # Used to access parent class's dynamically changing variables
         self.pkt_id = int(pkt_id_hex_str, 16)
         self.server_addr_port = server_addr_port
         self.send_delay = send_delay
         self.send_tlm = True
         
         self.udp_client_socket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
      
      def send(self):
      
         print("Starting telemetry client thread")
         
         while self.send_tlm:
         
            status_msg = struct.pack('hhhh128s',self.pkt_id,pi_ctrl.cmd_valid_cnt,pi_ctrl.cmd_invalid_cnt,
                                     pi_ctrl.cfs_running, pi_ctrl.cfs_app_tlm)
            self.udp_client_socket.sendto(status_msg, self.server_addr_port)
            time.sleep(self.send_delay)
 
      def terminate(self):
      
         self.send_tlm = False
         print("Terminating telemetry client")
         

if __name__ == "__main__":

   pi_ctrl = PiCtrl('pictrl.ini')
   pi_ctrl.execute()
   