from bottle import Bottle,route,run,template

app = Bottle()
with app:
  @route('/')
  def landing():
    return '''
          <h3>Configure Kovri Testnet</h3>
          <hr />
          <br />

          <a href="/config">configure</a>

          <br />

          <h3>Log Monitoring</h3>
          <hr />
          
          <p>
            Every kovri instance will provide real-time logging via named pipes (unless disabled)."<br />
            These pipes are located in their respective directories."<br />
            <br />
            Example: /tmp/testnet/kovri_010/log_pipe<br />
            <br />
            You can \"poll\" this output by simply cat\'ing the pipe:<br />
            <br />
            $ cat /tmp/testnet/kovri_010/log_pipe<br />
          </p>
    '''

  @route('/config')
  def configure():
    return '''
         <form action="/config" method="post">
            <label for="kovri_workspace">Kovri Workspace: </label>
            <input name="kovri_workspace" type="text" value="/tmp/kovri/contrib/testnet/testnet" /><br />

            <label for="kovri_network">Kovri network: </label>
            <input name="kovri_network" type="text" value="kovri-testnet" /><br />

            <label for="kovri_repo">Local kovri repository: </label>
            <input name="kovri_repo" type="text" value="/tmp/kovri" /><br />

            <label for="kovri_image">Kovri docker image: </label>
            <input name="kovri_image" type="text" value="geti2p/kovri:b1d505b2" /><br />

            <label for="kovri_web_image">Kovri web image: </label>
            <input name="kovri_web_image" type="text" value="httpd:2.4" /><br />

            <label for="kovri_dockerfile">Kovri docker file: </label>
            <input name="kovri_dockerfile" type="text" value="Dockerfile.alpine" /><br />

            <label for="kovri_web_dockerfile">Kovri web docker file: </label>
            <input name="kovri_web_dockerfile" type="text" value="Dockerfile.apache" /><br />

            <label for="kovri_bin_args">Kovri binary arguments: </label>
            <input name="kovri_bin_args" type="text" value="--floodfill 1 --enable-su3-verification 0 --log-auto-flush 1 --enable-https 0" /><br />

            <label for="kovri_fw_bin_args">Kovri firewall arguments: </label>
            <input name="kovri_fw_bin_args" type="text" value="--floodfill 0 --enable-su3-verification 0 --log-auto-flush 1" /><br />

            <label for="kovri_util_args">Kovri util arguments: </label>
            <input name="kovri_util_args" type="text" value="--floodfill 1 --bandwidth P" /><br />

            <hr />

            <label for="kovri_nb_base">Kovri instances: </label>
            <input name="kovri_instances" type="text" value="1" /><br />

            <label for="kovri_nb_fw">Kovri firewalled instances: </label>
            <input name="kovri_nb_fw" type="text" value="0" /><br />

            <label for="kovri_stop_timeout">Kovri stop timeout: </label>
            <input name="kovri_stop_timeout" type="text" value="0" /><br />

            <hr />

            <label for="kovri_buld_image">Build kovri image?: </label>
            <input name="kovri_build_image" type="checkbox" value="y" /><br />

            <label for="kovri_build_web_image">Build web image?: </label>
            <input name="kovri_build_web_image" type="checkbox" value="y" /><br />

            <label for="kovri_use_repo_bins">Use repo binaries?: </label>
            <input name="kovri_use_repo_bins" type="checkbox" value="y" /><br />

            <label for="kovri_build_repo_bins">Build repo binaries inside docker container?: </label>
            <input name="kovri_build_repo_bins" type="checkbox" value="y" /><br />

            <label for="kovri_cleanup">Cleanup previous testnet?: </label>
            <input name="kovri_cleanup" type="checkbox" value="y" /><br />

            <label for="kovri_use_named_pipes">Use name pipes for logging?: </label>
            <input name="kovri_use_named_pipes" type="checkbox" value="y" /><br />

            <label for="kovri_disable_monitoring">Disable logging?: </label>
            <input name="kovri_disable_monitoring" type="checkbox" value="y" /><br />

            <br />

            <input value="Configure" type="submit" />
        </form>
    ''' 
