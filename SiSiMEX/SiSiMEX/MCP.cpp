#include "MCP.h"
#include "UCP.h"
#include "AgentContainer.h"


enum State
{
	ST_INIT,
	ST_REQUESTING_MCCs,
	// DONE: Add other states
	ST_SEND_NEGOTIATION_REQUEST_MCC,
	ST_WAIT_NEGOTIATION_REQUEST_ACK,
	ST_NEGOTIATION_UCP,
	ST_NEGOTIATION_END
};

MCP::MCP(Node *node, uint16_t itemId, int layer) :
	Agent(node),
	_itemId(itemId),
	_negotiationAgreement(false), layer(layer)
{
	setState(ST_INIT);
}

MCP::~MCP()
{
}

void MCP::update()
{
	switch (state())
	{
	case ST_INIT:
	{
		queryMCCsForItem(_itemId);
		setState(ST_REQUESTING_MCCs);
		break;
	}
	// DONE: Handle other states
	case ST_SEND_NEGOTIATION_REQUEST_MCC:
	{
		sendNegotiationRequest(_mccRegisters[_mccRegisterIndex]);
		setState(ST_WAIT_NEGOTIATION_REQUEST_ACK);
		break;
	}
	case ST_NEGOTIATION_UCP:
	{
		if(_ucp->negotiationFinished())
		{
			_negotiationAgreement = _ucp->negotiationAgreement();
			if (!_negotiationAgreement && ++_mccRegisterIndex < _mccRegisters.size())
				setState(ST_SEND_NEGOTIATION_REQUEST_MCC);
			else
				setState(ST_NEGOTIATION_END);
		}
		break;
	}
	default:;
	}
}

void MCP::finalize()
{
	destroyChildUCP();
	finish();
}

void MCP::OnPacketReceived(TCPSocketPtr socket, const PacketHeader &packetHeader, InputMemoryStream &stream)
{
	const PacketType packetType = packetHeader.packetType;
	if (state() == ST_REQUESTING_MCCs && packetType == PacketType::ReturnMCCsForItem)
	{
		iLog << "OnPacketReceived PacketType::ReturnMCCsForItem " << _itemId;

		// Read the packet
		PacketReturnMCCsForItem packetData;
		packetData.Read(stream);

		if(!packetData.mccAddresses.empty())
		{
			for (auto &mccdata : packetData.mccAddresses)
			{
				uint16_t agentId = mccdata.agentId;
				const std::string &hostIp = mccdata.hostIP;
				uint16_t hostPort = mccdata.hostPort;
				iLog << " - MCC: " << agentId << " - host: " << hostIp << ":" << hostPort;
			}

			// Collect MCC compatible from YP
			_mccRegisters.swap(packetData.mccAddresses);

			// Select MCC to negociate
			_mccRegisterIndex = 0;
			setState(ST_SEND_NEGOTIATION_REQUEST_MCC);
		}
		else
		{
			setState(ST_NEGOTIATION_END);
		}

		socket->Disconnect();
	}
	// DONE: Handle other responses
	else if(state()== ST_WAIT_NEGOTIATION_REQUEST_ACK && packetType == PacketType::SendNegotiationRequestMCCAck)
	{
		AgentLocation uccLoc;
		uccLoc.agentId = packetHeader.srcAgentId;
		uccLoc.hostIP = socket->RemoteAddress().GetIPString();
		uccLoc.hostPort = LISTEN_PORT_AGENTS;

		createChildUCP(uccLoc);

		setState(ST_NEGOTIATION_UCP);
		socket->Disconnect();
	}
}

bool MCP::negotiationFinished() const
{
	// DONE
	return state() == ST_NEGOTIATION_END;
}

bool MCP::negotiationAgreement() const
{
	// DONE
	return _negotiationAgreement;
}


bool MCP::queryMCCsForItem(int itemId)
{
	// Create message header and data
	PacketHeader packetHead;
	packetHead.packetType = PacketType::QueryMCCsForItem;
	packetHead.srcAgentId = id();
	packetHead.dstAgentId = -1;
	PacketQueryMCCsForItem packetData;
	packetData.itemId = _itemId;

	// Serialize message
	OutputMemoryStream stream;
	packetHead.Write(stream);
	packetData.Write(stream);

	// 1) Ask YP for MCC hosting the item 'itemId'
	return sendPacketToYellowPages(stream);
}

bool MCP::sendNegotiationRequest(const AgentLocation &mccRegister)
{
	// DONE

	PacketHeader pckHeader;
	pckHeader.packetType = PacketType::SendNegotiationRequestMCC;
	pckHeader.srcAgentId = id();
	pckHeader.dstAgentId = mccRegister.agentId;
	PacketNegotiationRequest pckData;
	pckData.itemId = _itemId;

	OutputMemoryStream stream;
	pckHeader.Write(stream);
	pckData.Write(stream);

	return sendPacketToHost(mccRegister.hostIP, mccRegister.hostPort, stream);
}

void MCP::createChildUCP(const AgentLocation &uccLoc)
{
	// DONE
	_ucp = std::make_shared<UCP>(node(), _itemId, uccLoc, layer);
	g_AgentContainer->addAgent(_ucp);
}

void MCP::destroyChildUCP()
{
	// DONE
	if (_ucp)
		_ucp->finalize();
}
