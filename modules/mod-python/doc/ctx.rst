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


      (Read only attribute) (Number) returns the channel number.

   .. attribute:: vortex_ctx

      (Read only attribute) (vortex.Ctx) Returns associated vortex.Ctx object.


 
      
    
