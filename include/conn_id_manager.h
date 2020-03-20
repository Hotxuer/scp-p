#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <unordered_set>

/**
   * \class ConnidManager
   * \brief this class is used to assign and manage the connection ID
  */
class ConnidManager{
public:
    /**
    * \brief get the only 32 bits connection ID
    * \return 32 bits connection id
    */
    static uint32_t getConnID();

    /**
    * \brief delete the given 32 bits connection ID
    */
    static void delConnID(uint32_t connID);

    /**
    * store the 32 bits connection ID in client only
    * the connection id represents the client device
    */
    static int local_conn_id;
private:
    /**
    * \brief generate the 32 bits random number
    * \return 32 bits random number
    */
    static uint32_t getNewRandom32();

    /**
    * store the 32 bits connection ID
    * for server, this unordered_set collect all the connection ID of its clients
    */
    static std::unordered_set<uint32_t> ConnID_Manager;    
};
