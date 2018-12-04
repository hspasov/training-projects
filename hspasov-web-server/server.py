import os
import socket
from config import CONFIG
from log import log, DEBUG, INFO
from worker import Worker


class Server:
    def __init__(self):
        log.error(DEBUG)

        self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._accept_lock_fd = open('accept_lock', 'w')

    def run(self):
        log.error(DEBUG)

        is_initialized = False
        pid = None

        self._socket.bind((CONFIG['host'], CONFIG['port']))
        log.error(DEBUG, msg='socket bound: {0}:{1}'.format(CONFIG['host'],
                                                            CONFIG['port']))

        self._socket.listen(CONFIG['backlog'])
        log.error(DEBUG,
                  msg='listening... backlog: {0}'.format(CONFIG['backlog']))

        while True:
            try:
                i = 0
                while ((is_initialized and i < 1) or
                       (not is_initialized and i < CONFIG['workers'])):
                    i += 1

                    pid = os.fork()

                    if pid == 0:  # child process
                        try:
                            log.init_access_log_file()

                            worker = Worker(self._socket, self._accept_lock_fd)
                            worker.start()
                        except Exception as error:
                            log.error(INFO, msg=error)
                        finally:
                            os._exit(os.EX_SOFTWARE)
                    else:  # parent process
                        log.error(DEBUG, msg='New child created with pid {0}'.format(pid))  # noqa
                pid, exit_status = os.wait()

                log.error(DEBUG, var_name='pid', var_value=pid)
                log.error(DEBUG, var_name='exit_status', var_value=exit_status)
                log.error(DEBUG, var_name='WEXITSTATUS', var_value=os.WEXITSTATUS(exit_status))
                log.error(DEBUG, var_name='WTERMSIG', var_value=os.WTERMSIG(exit_status))

                is_initialized = True

            except OSError as error:
                log.error(DEBUG, msg=error)
            finally:
                if pid is not None and pid == 0:
                    log.close_access_log_file()
