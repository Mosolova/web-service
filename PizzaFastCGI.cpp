#include <fastcgi2/component.h>
#include <fastcgi2/component_factory.h>
#include <fastcgi2/handler.h>
#include <fastcgi2/request.h>

#include <iostream>
#include <sstream>

#include <stdio.h>
#include <cstring>
#include <algorithm> 
#include <iterator>
#include <vector>

#include <cstdlib>
#include "mongo/client/dbclient.h"
#include "mongo/bson/bson.h"

class PizzaFastCGI : virtual public fastcgi::Component, virtual public fastcgi::Handler
{
    public:

        mongo::DBClientConnection conn;

        PizzaFastCGI(fastcgi::ComponentContext *context) :
                fastcgi::Component(context)
        {

        }

        virtual void onLoad()
        {
            mongo::client::initialize();
            try {
                conn.connect("localhost");
                conn.ensureIndex("test.orders", mongo::fromjson("{order_id:1}"));
            } catch( const mongo::DBException &e ) {
                std::cout << "connection failed" << std::endl;
            }          
        }

        virtual void onUnload()
        {

        }

        bool isNumber(const std::string& s)
        {
            std::string::const_iterator it = s.begin();
            while (it != s.end() && std::isdigit(*it)) ++it;
            return !s.empty() && it == s.end();
        }

        void addOrder(mongo::BSONObj newObj, std::stringstream &stream){
        
            try {
                conn.insert("test.orders", newObj, true);
                if (conn.getLastError().length() != 0){
                    stream << "Insert error\n" << conn.getLastError() << std::endl;    
                } else {
                    stream << "Insert done successfully\n";    
                }
            } catch (const mongo::DBException &e) {
                std::cout<< "DB insert error\n" << conn.getLastError() << std::endl;
                stream << "DB insert error\n" << conn.getLastError() << std::endl;;
            }
        }

        void getOrder(std::string &order_id, std::stringstream &stream){      

            mongo::BSONObj p = mongo::BSONObjBuilder().append("order_id", order_id).obj();

            try {
                std::auto_ptr<mongo::DBClientCursor> cursor = conn.query("test.orders", p);                        
                while (cursor->more()){
                    stream << cursor->next().toString() << std::endl;
                }   
            } catch (const mongo::DBException &e) {
                std::cout<< "DB query error\n" << conn.getLastError() << std::endl;
                stream << "DB query error\n" << conn.getLastError() << std::endl;
            }
        }

        void listOrders(std::stringstream &stream){      
            
            try {
                std::auto_ptr<mongo::DBClientCursor> cursor = conn.query("test.orders", mongo::BSONObj());   
                while (cursor->more()){
                    stream << cursor->next().toString() << std::endl;
                }   
            } catch (const mongo::DBException &e) {
                std::cout<< "DB query error\n" << conn.getLastError() << std::endl;
                stream << "DB query error\n" << conn.getLastError() << std::endl;
            }
        }

        virtual void handleRequest(fastcgi::Request *request, fastcgi::HandlerContext *context)
        {
                std::stringstream stream;
                
                if(request->getRequestMethod() == "GET") {
                    
                    std::string str_url = request->getURI();
                    std::istringstream iss(str_url);
                    std::string token;
                    std::string order;                    
                    bool showAllOrders;
                    
                    while(std::getline(iss, token, '/')) {
                        if (token == "pizza"){
                            showAllOrders = true;
                            continue; 
                        } 
                        if (token == "order"){
                            showAllOrders = false;
                            if (std::getline(iss, token, '/') && isNumber(token)){                                
                                order = token;       
                            }
                        }
                    }
                    
                    if (showAllOrders){
                        listOrders(stream);
                    } else if (order.length() > 0){
                        getOrder(order, stream);
                    }

                } else if(request->getRequestMethod() == "POST") {
                    fastcgi::DataBuffer buf = request->requestBody();                    
                    std::string s;
                    buf.toString(s);
                    addOrder(mongo::fromjson(s), stream);
                }
                
                request->setContentType("application/json");    
                request->setHeader("Simple-Header", "Reply from PizzaFastCGI");
                std::stringbuf buffer(stream.str());       
                request->write(&buffer);
        }
};

FCGIDAEMON_REGISTER_FACTORIES_BEGIN()
FCGIDAEMON_ADD_DEFAULT_FACTORY("PizzaFastCGIFactory", PizzaFastCGI)
FCGIDAEMON_REGISTER_FACTORIES_END()
