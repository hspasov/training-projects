import os
import fcntl
import traceback
from http_meta import RequestMeta
from log import log, INFO, DEBUG, TRACE
from config import CONFIG
from client_connection import ClientConnection
from web_server_utils import resolve_static_file_path


class Worker:
    def __init__(self, socket, accept_lock_fd):
        log.error(DEBUG)

        self._socket = socket
        self._accept_lock_fd = accept_lock_fd

    def start(self):
        log.error(DEBUG)

        while True:
            client_conn = None

            try:
                client_conn = self.accept()

                # may send response to client in case of invalid
                # request
                client_conn.receive_meta()

                if client_conn.state != (
                    ClientConnection.State.RECEIVING
                ):
                    continue

                log.error(TRACE, msg='resolving file_path...')

                assert isinstance(client_conn.req_meta, RequestMeta)
                assert isinstance(client_conn.req_meta.target, str)

                # ignoring query params
                req_target_path = client_conn.req_meta.target \
                    .split('?', 1)[0]
                log.error(DEBUG, var_name='req_target_path',
                            var_value=req_target_path)

                file_path = os.path.realpath(req_target_path)

                log.error(DEBUG, var_name='file_path',
                            var_value=file_path)

                log.error(TRACE, msg=('requested file in web server ' +
                                        'document root'))

                # TODO make cgi-bin not accessible

                if file_path.startswith(CONFIG['cgi_dir']):
                    client_conn.serve_cgi_script(file_path)
                else:
                    client_conn.serve_static_file(
                        resolve_static_file_path(file_path)
                    )
                    log.error(DEBUG, msg='after serve static file')
            except ConnectionError as error:
                log.error(DEBUG, msg='ConnectionError')
                log.error(DEBUG, msg=error)
            except OSError as error:
                log.error(DEBUG, msg='OSError')
                log.error(DEBUG, msg=error)

                if client_conn is not None:
                    client_conn.send_meta(b'503')
            except Exception as error:
                log.error(INFO, msg='Exception')
                log.error(INFO, msg=error)
                log.error(INFO, msg=str(traceback.format_exc()))
                log.error(DEBUG, msg=error)

                if client_conn is not None:
                    client_conn.send_meta(b'500')
            finally:
                if client_conn is not None:
                    try:
                        client_conn.shutdown()
                        client_conn.close()
                    except Exception as error:
                        log.error(DEBUG, msg=error)

                    if client_conn.req_meta is None:
                        req_line = None
                        user_agent = None
                    else:
                        req_line = client_conn.req_meta.req_line_raw
                        user_agent = client_conn.req_meta.user_agent

                    log.access(
                        1,
                        remote_addr='{0}:{1}'.format(client_conn.remote_addr,
                                                        client_conn.remote_port),
                        req_line=req_line,
                        user_agent=user_agent,
                        status_code=client_conn.res_meta.status_code,
                        content_length=client_conn.res_meta.content_length,
                    )

    def accept(self):
        log.error(TRACE, msg='ready to accept connection')

        fcntl.lockf(self._accept_lock_fd, fcntl.LOCK_EX)
        log.error(DEBUG, msg='locked')
        conn, addr = self._socket.accept()
        log.error(DEBUG, msg='accepted')
        fcntl.lockf(self._accept_lock_fd, fcntl.LOCK_UN)

        log.error(TRACE, msg='connection accepted')
        log.error(DEBUG, var_name='conn', var_value=conn)
        log.error(DEBUG, var_name='addr', var_value=addr)

        return ClientConnection(conn, addr)

    def stop(self):
        log.error(DEBUG)
        self._socket.close()
        self._accept_lock_fd.close()
