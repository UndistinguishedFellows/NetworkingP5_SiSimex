#include "MCC.h"
#include "UCC.h"
#include "AgentContainer.h"

enum State
{
	ST_INIT,
	ST_REGISTERING,
	ST_IDLE,

	// DONE: Add other states ...
	ST_NEGOTIATION,
	
	ST_UNREGISTERING,
	ST_FINISHED
};

MCC::MCC(Node *node, uint16_t contributedItemId, uint16_t constraintItemId) :
	Agent(node),
	_contributedItemId(contributedItemId),
	_constraintItemId(constraintItemId)
{
	setState(ST_INIT);
}


MCC::~MCC()
{
}

void MCC::update()
{
	switch (state())
	{
	case ST_INIT:
	{
		if (registerIntoYellowPages()) {
			setState(ST_REGISTERING);
		}
		else {
			setState(ST_FINISHED);
		}
		break;
	}
	// DONE: Handle other states
	case ST_NEGOTIATION:
	{
		if(_ucc->negotiationFinished())
		{
			if(_ucc->negotiationAgreement())
			{
				//iLog << "MCC [ID: " << id() << "]: Set state - Unregistering.";
				setState(ST_UNREGISTERING);
				unregisterFromYellowPages();
			}
			else
			{
				//iLog << "MCC [ID: " << id() << "]: Set state - Idle.";
				setState(ST_IDLE);
			}
		}
	}	
	case ST_FINISHED:
		finish();
	}
}

void MCC::finalize()
{
	// DONE
	destroyChildUCC();
}


void MCC::OnPacketReceived(TCPSocketPtr socket, const PacketHeader &packetHeader, InputMemoryStream &stream)
{
	const PacketType packetType = packetHeader.packetType;
	if (state() == ST_REGISTERING && packetType == PacketType::RegisterMCCAck) 
	{
		setState(ST_IDLE);
		socket->Disconnect();
	}
	else if (state() == ST_UNREGISTERING && packetType == PacketType::UnregisterMCCAck) 
	{
		setState(ST_FINISHED);
		socket->Disconnect();
	}
	else if(state() == ST_IDLE && packetType == PacketType::SendNegotiationRequestMCC)
	{
		//DONE: handle other requests
		//iLog << "MCC [ID: " << id() << "]: Set state - Nogotiating with UCC.";
		setState(ST_NEGOTIATION);
		createChildUCC();
		
		// Send response

		OutputMemoryStream outStream;
		PacketHeader outHeader;
		outHeader.packetType = PacketType::SendNegotiationRequestMCCAck;
		outHeader.srcAgentId = _ucc->id();
		outHeader.dstAgentId = packetHeader.srcAgentId;
		outHeader.Write(outStream);

		socket->SendPacket(outStream.GetBufferPtr(), outStream.GetSize());
	}
}

bool MCC::negotiationFinished() const
{
	// DONE
	return _ucc ? _ucc->negotiationFinished() : false;
}

bool MCC::negotiationAgreement() const
{
	// DONE
	return _ucc->negotiationAgreement();
}

bool MCC::registerIntoYellowPages()
{
	// Create message header and data
	PacketHeader packetHead;
	packetHead.packetType = PacketType::RegisterMCC;
	packetHead.srcAgentId = id();
	packetHead.dstAgentId = -1;
	PacketRegisterMCC packetData;
	packetData.itemId = _contributedItemId;

	// Serialize message
	OutputMemoryStream stream;
	packetHead.Write(stream);
	packetData.Write(stream);

	return sendPacketToYellowPages(stream);
}

void MCC::unregisterFromYellowPages()
{
	// Create message
	PacketHeader packetHead;
	packetHead.packetType = PacketType::UnregisterMCC;
	packetHead.srcAgentId = id();
	packetHead.dstAgentId = -1;
	PacketUnregisterMCC packetData;
	packetData.itemId = _contributedItemId;

	// Serialize message
	OutputMemoryStream stream;
	packetHead.Write(stream);
	packetData.Write(stream);

	sendPacketToYellowPages(stream);
}

void MCC::createChildUCC()
{
	// DONE
	if(_contributedItemId != NULL_ITEM_ID)
		_ucc = std::make_shared<UCC>(node(), _contributedItemId, _constraintItemId);
	else
		_ucc = std::make_shared<UCC>(node(), _contributedItemId);

	g_AgentContainer->addAgent(_ucc);
}

void MCC::destroyChildUCC()
{
	// DONE
	if (_ucc) 
		_ucc->finalize();
}
