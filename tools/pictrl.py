import sys
import time
import configparser
import socket
import threading
import struct
import subprocess
import os.path
import logging
from datetime import datetime

#
# Provides a UDP connection that manages a simple pisat command & telemetry control
# interface for managing OSK's Rasberry Pi cfS target. pictrl.ini defines configuration
# parameters that are read during initialization. 
#
# See OSK's pi-sat Quick Start for instructions on how to automatically run this program
# when the Pi starts
#
class PiCtrl:

   NULL_CFS_APP_STR = "cFS not running"
   
   def __init__(self, IniFile):
      logging.basicConfig(filename='/tmp/pictrl.log',level=logging.DEBUG)
      self.log_info_event("***** Starting PiCtl %s *****" % datetime.now().strftime("%m/%d/%y-%H:%M"))
      self.event_msg = "" 
      self.event_msg_tlm = bytes(self.event_msg.ljust(128, '\0'), 'utf-8')
      
      self.config = configparser.ConfigParser()
      self.config.read(IniFile)
      self.exec = self.config['exec']
      self.network = self.config['network']   
      self.cfs = self.config['cfs']
      
   
      self.commands = { 'PI_NOOP': self.noop_cmd(self), 'PI_ENA_TLM': self.ena_tlm_cmd(self), 'PI_CTRL_EXIT': self.exit_cmd(self),
                        'PI_HALT': self.pi_halt_cmd(self), 'PI_REBOOT': self.pi_reboot_cmd(self),
                        'CFS_START': self.cfs_start_cmd(self), 'CFS_STOP': self.cfs_stop_cmd(self) } 
      
      self.cmd_valid_cnt = 0
      self.cmd_invalid_cnt = 0
      self.tlm_client = None
      
      self.cmd_ip_addr  = self.network['cmd-tlm-ip-addr']
      self.cmd_port     = self.config.getint('network','cmd-port')
      self.cmd_buf_size = self.config.getint('network','cmd-buf-size')
      self.log_info_event("Creating socket for IP address " + self.cmd_ip_addr + " port " + str(self.cmd_port))
      
      self.cmd_socket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)      
      #~self.cmd_socket.bind((self.cmd_ip_addr, self.cmd_port))
      self.cmd_socket.bind(('', self.cmd_port))
      
      #~mreq = struct.pack('4sl',socket.inet_aton(self.cmd_ip_addr), socket.INADDR_ANY)
      #~mreq = struct.pack('4sl',socket.inet_aton("224.0.0.0"), socket.INADDR_ANY)
      #~self.cmd_socket.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
      #~self.cmd_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
      
      self.cfs_running = False
      self.cfs_process = None
      self.clear_cfs_app_str()
      
      
   def execute(self):
      while True:
         try:
            
            msg_addr_array = self.cmd_socket.recvfrom(self.cmd_buf_size)
            message = msg_addr_array[0]
            address = msg_addr_array[1]

            cmd_str = message.decode().strip()

            if cmd_str in self.commands:
               if (self.commands[cmd_str].execute()):
                  self.cmd_valid_cnt += 1
               else:
                  self.cmd_invalid_cnt += 1
            else:
               self.log_error_event("Received undefined command %s" % cmd_str)
               self.cmd_invalid_cnt += 1
                              
         except KeyboardInterrupt:
            terminate('Intercepted Control-C')


   def ena_tlm(self):
      self.tlm_client = self.Telemetry(self, (self.network['cmd-tlm-ip-addr'], self.config.getint('network','tlm-port')), 
                                       self.config.getint('exec','tlm-delay'), self.exec['tlm-pkt-id'])
      tlm_thread = threading.Thread(target=self.tlm_client.send)
      tlm_thread.start()

   def terminate(self, exit_str):
      self.log_info_event('Exiting PiSat Control: ' + exit_str)
      if pi_ctrl.cfs_running:
         self.commands['CFS_STOP'].execute()
      self.tlm_client.terminate()
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
      self.cfs_app_str = PiCtrl.NULL_CFS_APP_STR
      self.cfs_app_tlm = bytes(self.cfs_app_str.ljust(128, '\0'), 'utf-8')


   def log_info_event(self, msg_str):
      logging.info(msg_str)
      self.log_event(msg_str)
      
   def log_error_event(self, msg_str):
      logging.error(msg_str)
      self.log_event(msg_str)

   def log_event(self, msg_str):
      self.event_msg = msg_str
      self.event_msg_tlm = bytes(self.event_msg.ljust(128, '\0'), 'utf-8')
      print(msg_str)


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
   
   #
   # PiCtl Executive Commands
   #
   class noop_cmd(Command):
      
      def virtual_execute(self):
         self.pi_ctrl.log_info_event("No operation command received")
         return True

   class ena_tlm_cmd(Command):
      
      def virtual_execute(self):
         self.pi_ctrl.ena_tlm()
         return True

   class exit_cmd(Command):
      
      def virtual_execute(self):
         pi_ctrl.terminate('Received Exit Command')
         return True
   #
   # cFS Executive Commands
   #
   class cfs_start_cmd(Command):
      
      def virtual_execute(self):
   
         ret_status = False
         
         try:
             
            if pi_ctrl.cfs_running:
               
               self.pi_ctrl.log_error_event("Start cFS command rejected. The cFS is already running, pid = %d." % pi_ctrl.cfs_process.pid)
            
            else:
               pi_ctrl.create_cfs_app_str(pi_ctrl.cfs['target-path'])            
               pi_ctrl.cfs_process = subprocess.Popen([pi_ctrl.cfs['target-binary']],
                                                      cwd=pi_ctrl.cfs['target-path'],
                                                      stdout=subprocess.PIPE,
                                                      stderr=subprocess.STDOUT,
                                                      shell=False)
               
               pi_ctrl.cfs_running = True
               ret_status = True
               self.pi_ctrl.log_info_event("Start cFS, pid = %d" % pi_ctrl.cfs_process.pid)
               #~process_output, _ = pi_ctrl.cfs_process.communicate()
               #~self.pi_ctrl.log_info_event("Start cFS process out: " + process_output.decode())
               
         except (OSError, subprocess.CalledProcessError) as exception:
            self.pi_ctrl.log_error_event('Start cFS failed with exception: ' + str(exception))
          
         return ret_status
         

   class cfs_stop_cmd(Command):
      
      def virtual_execute(self):
      
         ret_status = False

         if pi_ctrl.cfs_running:
            pi_ctrl.clear_cfs_app_str()
            self.pi_ctrl.log_info_event("Stopping cFS, pid = %d" % pi_ctrl.cfs_process.pid)
            subprocess.call(["kill", "-9", "%d" % pi_ctrl.cfs_process.pid])
            pi_ctrl.cfs_process.wait()
            pi_ctrl.cfs_running = False
            ret_status = True
         else:
            self.pi_ctrl.log_error_event("Stop cFS command rejected. The cFS is not running.")
         
         return ret_status


   #
   # Pi Executive Commands
   #

   class pi_halt_cmd(Command):
      
      def virtual_execute(self):
         subprocess.Popen('halt', shell=False)
         return True

   class pi_reboot_cmd(Command):
      
      def virtual_execute(self):
         subprocess.Popen('reboot', shell=False)
         return True

   #
   # Telemetry Inner Class
   #
   class Telemetry:
      def __init__(self, pi_ctrl, server_addr_port, send_delay, pkt_id_hex_str):
      
         self.pi_ctrl = pi_ctrl     # Used to access parent class's dynamically changing variables
         self.pkt_id = int(pkt_id_hex_str, 16)
         self.pkt_cnt = 0
         self.server_addr_port = server_addr_port
         self.send_delay = send_delay
         self.send_tlm = True
         
         self.tlm_socket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
      
      def send(self):
      
         self.pi_ctrl.log_info_event("Starting telemetry client thread")
         
         while self.send_tlm:
         
            self.pkt_cnt += 1
            status_msg = struct.pack('hhhhh128s128s',self.pkt_id,self.pkt_cnt,pi_ctrl.cmd_valid_cnt,pi_ctrl.cmd_invalid_cnt,
                                     pi_ctrl.cfs_running, pi_ctrl.event_msg_tlm, pi_ctrl.cfs_app_tlm)
            self.tlm_socket.sendto(status_msg, self.server_addr_port)
            time.sleep(self.send_delay)
 
      def terminate(self):
      
         self.send_tlm = False
         self.pi_ctrl.log_info_event("Terminating telemetry client")
         

if __name__ == "__main__":

   pi_ctrl = PiCtrl('/home/pi/pi-sat/tools/pictrl.ini')
   pi_ctrl.execute()
   