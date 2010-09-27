:mod:`tbc.Ctx` --- PyTurbulenceCtx class: interface to TurbulenceCtx object and other support functions
=======================================================================================================

.. currentmodule:: tbc


=====
Intro
=====



==========
Module API
==========

.. class:: Ctx

   .. method:: msg (content)
   
      Allows to record a turbulence info message (using turbulence msg ()). This is sent to the configured log and showed on the console according to the configuration.

      :param msg: The message to record
      :type  content: String

   .. method:: error (content)
   
      Records a turbulence error message (using error ()). This is sent to the configured log and showed on the console according to the configuration.

      :param content: The error message to record
      :type  content: String

   .. method:: find_con_by_id (conn_id)
   
      Allows to find a connection registered on the turbulence connection manager with the provided connection id. The method returns the connection (vortex.Connection) or None in case of failure. This python method provides access to turbulence_conn_mgr_find_by_id.

      :param conn_id: The connection unique identifer
      :type  conn_id: Int

      :rtype: Returns vortex.Connection or None if fails.

   .. method:: broadcast_msg (message, size, profile)
   
      This method is binding of turbulence_conn_mgr_broadcast_msg. See C API documentation for more info. The method allows to broadcast a message over all channels running the profile provided over all registered connections.

      :param message: The message to broadcast.
      :type  message: String

      :param size: Message size to broadcast
      :type  size: Int

      :param profile: The BEEP profile to use to select channels on registered connections.
      :type  message: String

      :rtype: Returns True in case or success, otherwise False is returned.

   .. attribute:: vortex_ctx

      (Read only attribute) (vortex.Ctx) Returns associated vortex.Ctx object.


 
      
    
