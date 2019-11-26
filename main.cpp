// basic-http-client.cpp
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <iostream>
#include <sstream>


using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams

using namespace std;


web::http::status_code _breakingStatus = status_codes::OK;
//utility::size64_t temp = 0;
constexpr auto CHUNK = 10485760; //10 MB;





pplx::task<http_response> make_task_request(http_client& client, method mtd, json::value const& jvalue)
{
	return (mtd == methods::GET || mtd == methods::HEAD) ?
		client.request(mtd, L"/restdemo") :
		client.request(mtd, L"/restdemo", jvalue);
}

void MakeRequest(http_client& client, method mtd, json::value const& jvalue)
{
	//make_task_request(client, mtd, jvalue)
	client.request(mtd, L"/restdemo", jvalue)
		.then([](http_response response)
			{
				if (response.status_code() != status_codes::OK)
				{
					printf("Received response status code:%u\n", response.status_code());
				}
				return pplx::task_from_result(json::value());
			})
		.then([](pplx::task<json::value> previousTask)
			{
				try
				{
					previousTask.wait();
				}
				catch (http_exception const& e)
				{
					wcout << e.what() << endl;
				}
			})

				.wait();
}




// Upload a file to an HTTP server.
pplx::task<void> UploadFileToHttpServerAsync(http_client& client, const wchar_t* fileName)
{
	using concurrency::streams::file_stream;
	using concurrency::streams::basic_istream;

	// To run this example, you must have a file named myfile.txt in the current folder. 
	// Alternatively, you can use the following code to create a stream from a text string. 
	// std::string s("abcdefg");
	// auto ss = concurrency::streams::stringstream::open_istream(s); 
	
	// Open stream to file.
	return file_stream<unsigned char>::open_istream(fileName).then([&](pplx::task<basic_istream<unsigned char>> previousTask)
		{
			try
			{
				Concurrency::streams::istream fileStream = previousTask.get();
	

				//utility::size64_t upsize = 4711u, downsize = 4711u;
				//int calls = 0;
				http_client_config config;
				config.set_chunksize(CHUNK);

				utility::string_t address = U("http://127.0.0.1:34568");
				http_client client(address, config);
		
				// Make HTTP request with the file stream as the body.
				http_request request(methods::PUT);
				request.headers().add(L"Filename", fileName);
				request.set_request_uri(L"myfiletoputonserver");
				request.set_body(fileStream);
				//temp = fileStream.streambuf().size();
				//utility::size64_t c = fileStream.streambuf().size();
				
				request.set_progress_handler([fileStream](message_direction::direction direction, utility::size64_t so_far) {
					utility::size64_t buf = fileStream.streambuf().size();

					if (direction == message_direction::upload)
					{
	
						//std::cout << "Call #" << calls << " : ";
						//cout.precision(4);
						cout.right;
						cout.width(4);
						cout << floor((double)so_far * 100 / buf) << " % \r";

						//upsize = so_far;
					}
						
					else
					{
						//downsize = so_far;
					}
					});

				return client.request(request).then([fileStream](pplx::task<http_response> previousTask)
					{		
						fileStream.close();
						
						std::wostringstream ss;
						try
						{
							web::http::http_response response = previousTask.get();
							_breakingStatus = response.status_code();
							if (_breakingStatus == status_codes::OK)
							{
								ss << L"Server returned returned status code " << response.status_code() << L"." << std::endl;
							}
						}
						catch (const http_exception & e)
						{
							ss << e.what() << std::endl;
						}
						std::wcout << ss.str();
					});
			}
			catch (const std::system_error & e)
			{
				std::wostringstream ss;
				ss << e.what() << std::endl;
				std::wcout << ss.str();

				// Return an empty task. 
				return pplx::task_from_result();
			}
		});
}


pplx::task<void> CheckProgress(http_client& client)
{	
	// Make the request and asynchronously process the response. 
	return client.request(methods::GET, L"isDownloading").then([](http_response response)
	{

			if (response.status_code() == status_codes::Accepted)
			{
				response
					.extract_json()
					.then([](pplx::task<json::value> task)
					{
						try
						{
							const json::value& jvalue = task.get();
							auto _msg = jvalue.as_object();
							wcout << "Uploading progress is " << _msg[L"progress"] << "%""\r";

						}
						catch (http_exception const& e)
						{
							wcout << e.what() << endl;
						}
					})
					.wait();
			
			}
	});
}


#ifdef _WIN32
int wmain(int argc, wchar_t* argv[])
#else
int main(int argc, char* argv[])
#endif
{
	utility::string_t port = U("34568");
	if (argc == 2)
	{
		port = argv[1];
	}

	utility::string_t address = U("http://127.0.0.1:");
	address.append(port);
	http_client client(address);

	std::string _clientName("");
	std::string _clientMessage("");

	std::cout << "Type the word ""EXIT"" to stop texting!!!" << std::endl << std::endl << "Enter your name: ";
	std::getline(std::cin, _clientName);
	if (_clientName == "EXIT" || _clientName == "exit")
	{
		return 0;
	}

	

	while (true)
	{
		std::cout << "Enter the file name: ";
		std::getline(std::cin, _clientMessage);

		if (_clientMessage == "EXIT" || _clientMessage == "exit")
		{
			break;
		}

		std::wstring wide_clientMessage = std::wstring(_clientMessage.begin(), _clientMessage.end());
		const wchar_t* _userMessage = wide_clientMessage.c_str();

		std::wstring wide_clientName = std::wstring(_clientName.begin(), _clientName.end());
		const wchar_t* _userName = wide_clientName.c_str();

		json::value postData;
		postData[L"name"] = json::value::string(_userName);
		postData[L"message"] = json::value::string(_userMessage);

		MakeRequest(client, methods::POST, postData);



		//Uploading and checking----------------------------------------

		std::wcout << L"Calling UploadFileToHttpServerAsync..." << std::endl;
		UploadFileToHttpServerAsync(client, _userMessage).wait();

/*
		std::wcout << L"Calling CheckProgress..." << std::endl;
		while (_breakingStatus != status_codes::Created)
		{
			CheckProgress(client).wait();
		}
*/
		if (_breakingStatus == status_codes::Created) // Created means that the file was fully transmitted 
		{
			std::wcout << "File uploading has been completed successfully" << std::endl << std::endl << std::endl;
		}

		_breakingStatus = status_codes::OK; //reset breaking status
		//----------------------------------------------------------------
		
	}





	


	return 0;
}