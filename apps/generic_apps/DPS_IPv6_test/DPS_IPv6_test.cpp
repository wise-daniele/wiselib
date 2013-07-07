/*
* File: DPS_IPv6_test.cpp
* Author: Daniel Gehberger - GSoC 2013 - DPS project
*/

#ifdef ISENSE
//Needed for iSense in order to avoid linker error with the MapStaticVector class
extern "C" void assert(int) { }
#endif

#include "external_interface/external_interface.h"
#include "radio/DPS/IPv6/DPS_ipv6_stack.h"

#ifdef SHAWN
#include "external_interface/shawn/shawn_testbedservice_uart.h"
#endif

typedef wiselib::OSMODEL Os;
typedef Os::Radio Radio;
typedef Os::Radio::node_id_t node_id_t;

#ifdef DPS_IPv6_SKELETON
	#ifdef SHAWN
	typedef wiselib::ShawnTestbedserviceUartModel<wiselib::OSMODEL> Uart;
	#elif defined ( ISENSE )
	//In case of iSense use the large packet uart --> uint16 for packet length
	#include "external_interface/isense/isense_com_uart_largepackets.h"
	typedef wiselib::iSenseSerialComUartModelLargePackets<Os> Uart;
	#else
	typedef Os::Uart Uart;
	#endif
	
	typedef wiselib::DPS_IPv6Stack<Os, Radio, Os::Debug, Os::Timer, Os::Clock, Uart, Os::Rand> DPS_IPv6Stack;
#elif defined DPS_IPv6_STUB
	typedef wiselib::DPS_IPv6Stack<Os, Radio, Os::Debug, Os::Timer, Os::Rand> DPS_IPv6Stack;
#endif

typedef wiselib::IPv6Address<Radio, Os::Debug> IPv6Address_t;
typedef wiselib::UDPSocket<IPv6Address_t> UDPSocket_t;

uint8_t my_prefix[8] = IPv6_PREFIX

class lowpanApp
{
public:
	typedef Radio::block_data_t block_data_t;
	typedef Radio::node_id_t node_id_t;
	typedef Radio::size_t size_t;
	typedef Radio::message_id_t message_id_t;
	
	void init( Os::AppMainParameter& value )
	{
		radio_ = &wiselib::FacetProvider<Os, Os::Radio>::get_facet( value );
		timer_ = &wiselib::FacetProvider<Os, Os::Timer>::get_facet( value );
		debug_ = &wiselib::FacetProvider<Os, Os::Debug>::get_facet( value );
		rand_ = &wiselib::FacetProvider<Os, Os::Rand>::get_facet( value );
		
	#ifdef ISENSE

		uint8_t uart_size = 8;
		//For the SLIP support the extended addressing is needed on the UART
		#ifdef ISENSE_ENABLE_UART_16BIT
			uart_size = 16;
		#endif
	// --> Set the Radio Chanel in this line
		debug_->debug( "Booting with ID: %lx, radio channel: %i, %i-bit radio & %i-bit UART addresses\n", (long long unsigned)(radio_->id()), radio_->set_channel( 18 ), 8*sizeof(radio_->id()), uart_size);
		debug_->debug( "MEM: %i",  mem->mem_free() );
	#else
		debug_->debug( "Booting with ID: %llx\n", (long long unsigned)(radio_->id()));
	#endif
		
#ifdef DPS_IPv6_SKELETON
		clock_ = &wiselib::FacetProvider<Os, Os::Clock>::get_facet( value );
		uart_ = &wiselib::FacetProvider<Os, Uart>::get_facet( value );
		ipv6_stack_.init(*radio_, *debug_, *timer_, *uart_, *rand_, *clock_);
#elif defined DPS_IPv6_STUB
		ipv6_stack_.init(*radio_, *debug_, *timer_, *rand_);
#endif
		
		callback_id = ipv6_stack_.icmpv6.reg_recv_callback<lowpanApp,&lowpanApp::receive_echo_reply>( this );
		callback_id = ipv6_stack_.udp.reg_recv_callback<lowpanApp,&lowpanApp::receive_radio_message>( this );
		
// --> The Neighbor Discovery can be started with this line, it MUST be enabled for the SLIP communication
		//ipv6_stack_.icmpv6.ND_timeout_manager_function( NULL );
		
// --> for the border router the IPv6_SLIP MUST be defined in the lowpan_config.h
		
		//--------------------------------------------------------------------------------------
		//					Testing part
		//--------------------------------------------------------------------------------------
		
		/*
			Test destination
		*/
				
		IPv6Address_t destinationaddr;
// 		
	#ifdef DPS_IPv6_SKELETON
			node_id_t ll_id = radio_->id();
			destinationaddr.set_prefix( my_prefix, 64 );
			destinationaddr.set_long_iid(&ll_id, true);
			
			ipv6_stack_.interface_manager.set_prefix_for_interface( my_prefix, 0, 64 );
	#endif
// 		/*
// 			UDP test
// 			
// 			For the best compression use ports between 61616 and 61631
// 		*/
// 		
#ifdef DPS_IPv6_STUB
		{
			seq = 0;
			timer_->set_timer<lowpanApp, &lowpanApp::send>( rand()() % (0x1FFF) + 5000, this, NULL );
		}
#endif
		
		if( radio_->id() == 0x2144 )
		{
			//local_port, registered UDP callback ID, [remote_port], [remote_host]
			ipv6_stack_.udp.listen( 61617, callback_id );
			//ipv6_stack_.udp.print_sockets();
		}
		
	}
	
	void send( void* )
	{
		seq++;
		
		IPv6Address_t destinationaddr;
		node_id_t ll_id = 0x2144;

		destinationaddr.set_prefix( my_prefix, 64 );
		destinationaddr.set_long_iid(&ll_id, true);
		uint8_t mypayload[] = "UDPfrom: ____ #: __ ";
		//9-10-11-12
		//15-16
		mypayload[9] = destinationaddr.get_hex((radio_->id() >> 12) & 0xF);
		mypayload[10] = destinationaddr.get_hex((radio_->id() >> 8) & 0xF);
		mypayload[11] = destinationaddr.get_hex((radio_->id() >> 4) & 0xF);
		mypayload[12] = destinationaddr.get_hex((radio_->id() >> 0) & 0xF);
		
		mypayload[17] = destinationaddr.get_hex((seq >> 4) & 0xF);
		mypayload[18] = destinationaddr.get_hex((seq >> 0) & 0xF);
		
		//Set IPv6 header fields
// 			ipv6_stack_.ipv6.set_traffic_class_flow_label( 0, 42 );
		
		// local_port, remote_port, remote_host
		UDPSocket_t my_socket = UDPSocket_t( 61616, 61617, destinationaddr );
		ipv6_stack_.udp.send(my_socket,sizeof(mypayload),mypayload);
// 		ipv6_stack_.icmpv6.ping(destinationaddr);
		
		timer_->set_timer<lowpanApp, &lowpanApp::send>( 15000, this, NULL );
	}

	// --------------------------------------------------------------------
	void receive_radio_message( UDPSocket_t socket, uint16_t len, Os::Radio::block_data_t *buf )
	{
		//Drop the messages from this node --> only the IP addresses are compared
		if( socket != ipv6_stack_.udp.id() )
		{
			char str[43];
			debug_->debug( "Application layer received msg at %lx from %s", (long long unsigned)(radio_->id()), socket.remote_host.get_address(str) );
			debug_->debug( "    Size: %i Content: %s ", len, buf);
		}
	}

	// --------------------------------------------------------------------
	void receive_echo_reply( IPv6Address_t from, uint16_t len, Os::Radio::block_data_t *buf )
	{
		if( len == 1 && buf[0] == ipv6_stack_.icmpv6.ECHO_REPLY )
		{
			char str[43];
			debug_->debug( "Application layer received echo reply at %lx from %s", (long long unsigned)(radio_->id()), from.get_address(str) );
		}
	}

private:
	int callback_id;
	int seq;

	DPS_IPv6Stack ipv6_stack_;
	Radio::self_pointer_t radio_;
	Os::Timer::self_pointer_t timer_;
	Os::Debug::self_pointer_t debug_;
	Os::Rand::self_pointer_t rand_;
#ifdef DPS_IPv6_SKELETON
	Os::Clock::self_pointer_t clock_;
	Uart::self_pointer_t uart_;
#endif
	Os::Rand& rand()
	{ return *rand_; }
};
// --------------------------------------------------------------------------
wiselib::WiselibApplication<Os, lowpanApp> example_app;
// --------------------------------------------------------------------------
void application_main( Os::AppMainParameter& value )
{
	example_app.init( value );
}