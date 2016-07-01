/*********************************************************************************************************************
 * Copyright 2013-2014 Tobii Technology AB. All rights reserved.
 * Callbacks.hpp
 *********************************************************************************************************************/

#if !defined(__TOBII_TX_CLIENT_CPPBINDINGS_CALLBACKS__HPP__)
#define __TOBII_TX_CLIENT_CPPBINDINGS_CALLBACKS__HPP__

/*********************************************************************************************************************/

TX_NAMESPACE_BEGIN
	
/*********************************************************************************************************************/

typedef std::function<void (TX_CONNECTIONSTATE)> ConnectionStateChangedHandler;
typedef std::function<void (const std::unique_ptr<AsyncData>&)> AsyncDataHandler;

/*********************************************************************************************************************/

TX_NAMESPACE_END

/*********************************************************************************************************************/

#endif // !defined(__TOBII_TX_CLIENT_CPPBINDINGS_CALLBACKS__HPP__)

/*********************************************************************************************************************/
