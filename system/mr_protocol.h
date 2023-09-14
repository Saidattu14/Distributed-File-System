// #ifndef mr_protocol_h_
// #define mr_protocol_h_

// #include <string>
// #include <vector>

// #include "rpc.h"

// using namespace std;

// #define REDUCER_COUNT 4

// enum mr_tasktype {
// 	NONE = 0, // this flag means no task needs to be performed at this point
// 	MAP,
// 	REDUCE
// };

// class submitmapresponse1 {
// 	public:
// 	int index;
// };


// marshall &operator<<(marshall &m, const submitmapresponse1 &reply);
// unmarshall &operator>>(unmarshall &m, submitmapresponse1 &reply);

// class mr_protocol {
// public:
// 	typedef int status;
// 	enum xxstatus { OK, RPCERR, NOENT, IOERR };
// 	enum rpc_numbers {
// 		asktask = 0xa001,
// 		submittask = 0xb001,
// 		submitmaptask= 0xc001,
// 	};
// 	struct KeyVal {
// 		string key;
// 		string val;
// 	};

// 	struct AskTaskResponse {
// 		// Lab4: Your definition here.
//         int taskType;
//         int index;
// 		long long unsigned int inum;
// 	    long long unsigned int file_length;
// 	};

// 	// struct AskTaskRequest {
// 	// 	// Lab4: Your definition here.
// 	// };

// 	// struct SubmitTaskResponse {
// 	// 	// Lab4: Your definition here.
// 	// };

// 	// struct SubmitTaskRequest {
// 	// 	// Lab4: Your definition here.
// 	// };

// 	// struct SubmitMapTaskResponse {
// 	// 	// Lab4: Your definition here.
// 	//     string index = "jj";
// 	// };
// 	// // friend marshall &operator<<(marshall &m, const KeyVal &cmd) {
//     // // 	m<<cmd.key<<cmd.val;
//     // // 	return m;
// 	// // }
// 	// // friend unmarshall &operator>>(unmarshall &u, KeyVal cmd) {
// 	// // 	u>>cmd.key>>cmd.val;
// 	// // 	return u;
// 	// // }
	
// 	// friend marshall &operator<<(marshall &m1, const SubmitMapTaskResponse &cmd) {
// 	// 	m1<<cmd.index;
// 	// 	return m1;
// 	// }
// 	// friend unmarshall &operator>>(unmarshall &u1, SubmitMapTaskResponse &cmd) {
// 	// 	u1>>cmd.index;
// 	// 	return u1;
// 	// }
// };


	

// #endif


#ifndef mr_protocol_h_
#define mr_protocol_h_

#include <string>
#include <vector>

#include "rpc.h"

using namespace std;

#define REDUCER_COUNT 4

struct KeyVal {
    string key;
    string val;
};

enum mr_tasktype {
	NONE = 0, // this flag means no task needs to be performed at this point
	MAP,
	REDUCE
};

class mr_protocol {
public:
	typedef int status;
	enum xxstatus { OK, RPCERR, NOENT, IOERR };
	enum rpc_numbers {
		asktask = 0xa001,
		submittask,
		submitmaptask,
	};

	struct AskTaskResponse {
		// Lab4: Your definition here.
		int taskType;
        int index;
		long long unsigned int inum;
	    long long unsigned int file_length;
	};

	struct AskTaskRequest {
		// Lab4: Your definition here.
	};

	struct SubmitTaskResponse {

		// Lab4: Your definition here.
	};

	struct SubmitTaskRequest {
		int index;
		// Lab4: Your definition here.
	};

	struct SubmitMapTaskResponse {
		// Lab4: Your definition here.
		int index;

	};

};
class submitmapresponse1 {
	public:
	int index;
};
marshall &operator<<(marshall &m, const submitmapresponse1 &s);
unmarshall &operator>>(unmarshall &u, submitmapresponse1 &s);

inline unmarshall &
operator>>(unmarshall &u1, mr_protocol::AskTaskResponse &res) 
{
		u1 >> res.taskType;
		u1 >> res.index;
		u1 >> res.file_length;
		u1 >> res.inum ;
		return u1;
}

inline marshall &
operator<<(marshall &m1, const mr_protocol::AskTaskResponse &res) 
{
		m1 << res.taskType;
		m1 << res.index;
		m1 << res.file_length;
		m1 << res.inum ;
		return m1;
}

inline unmarshall &
operator>>(unmarshall &u1, mr_protocol::SubmitMapTaskResponse &res) 
{
		u1 >> res.index;
		return u1;
}


inline marshall &
operator<<(marshall &m1, const mr_protocol::SubmitMapTaskResponse &res) 
{
		m1 << res.index;
		return m1;
}

inline unmarshall &
operator>>(unmarshall &u1, mr_protocol::SubmitTaskRequest &res) 
{
		u1 >> res.index;
		return u1;
}


inline marshall &
operator<<(marshall &m1, const mr_protocol::SubmitTaskRequest &res) 
{
		m1 << res.index;
		return m1;
}


inline unmarshall &
operator>>(unmarshall &u1, KeyVal &res) 
{
		u1 >> res.key;
		u1 >> res.val;
		return u1;
}


inline marshall &
operator<<(marshall &m1, const KeyVal &res) 
{
		m1 << res.key;
		m1 << res.val;
		return m1;
}

#endif

