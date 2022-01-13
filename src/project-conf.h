/* ------ Network Config ------ */

// #undef NETSTACK_NETWORK
// #define NETSTACK_NETWORK rime_driver

#undef NETSTACK_CONF_MAC 
#define NETSTACK_CONF_MAC  csma_driver  //csma_driver //nullmac_driver // csma_driver

#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC  nullrdc_driver //nullrdc_driver // contikimac_driver // cxmac_driver  

#undef NETSTACK_CONF_FRAMER
#define NETSTACK_CONF_FRAMER framer_802154 // framer_nullmac

#undef NETSTACK_CONF_RADIO
#define NETSTACK_CONF_RADIO cc2420_driver


// #undef TIMESYNCH_CONF_ENABLED
// #define TIMESYNCH_CONF_ENABLED 1
