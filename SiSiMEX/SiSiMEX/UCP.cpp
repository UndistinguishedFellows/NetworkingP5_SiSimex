#include "UCP.h"
#include "MCP.h"
#include "AgentContainer.h"


enum State
{
	ST_INIT,
	// DONE: Add other states...
	ST_WAIT_ITEM_CONSTRAIN,
	ST_NEGOTIATION_END,
	ST_WAIT_NEGOTIATION
};

UCP::UCP(Node *node, uint16_t requestedItemId, const AgentLocation &uccLocation, int layer) :
	Agent(node),
	_requestedItemId(requestedItemId),
	_uccLocation(uccLocation),
	_negotiationAgreement(false), layer(layer)
{
	setState(ST_INIT);
}

UCP::~UCP()
{
}

void UCP::update()
{
	switch (state())
	{
	case ST_INIT:
	{
		requestItem();
		setState(ST_WAIT_ITEM_CONSTRAIN);
		break;
	}
	// DONE: Handle other states
	case ST_WAIT_NEGOTIATION:
	{
		if(_mcp->negotiationFinished())
		{
			_negotiationAgreement = _mcp->negotiationAgreement();
			endNegotiation(_negotiationAgreement);
			setState(ST_NEGOTIATION_END);
		}
	}

	default:;
	}
}

void UCP::finalize()
{
	destroyChildMCP();
	finish();
}

void UCP::OnPacketReceived(TCPSocketPtr socket, const PacketHeader &packetHeader, InputMemoryStream &stream)
{
	PacketType packetType = packetHeader.packetType;
	// DONE: Handle requests

	if(state() == ST_WAIT_ITEM_CONSTRAIN && packetType == PacketType::ResourceOfferToUCP)
	{
		setState(ST_NEGOTIATION_END);
		_negotiationAgreement = true;
		socket->Disconnect();
	}
	else if(state() == ST_WAIT_ITEM_CONSTRAIN && packetType == PacketType::ItemConstrain)
	{
		PacketItemConstrainToUCP pckData;
		pckData.Read(stream);

		if(layer < 5)
		{
			createChildMCP(pckData.itemId);
			setState(ST_WAIT_NEGOTIATION);
		}
		else
		{
			endNegotiation(socket, packetHeader, false);
			socket->Disconnect();
		}
	}
}

bool UCP::negotiationFinished() const {
	// DONE
	return state() == ST_NEGOTIATION_END;
}

bool UCP::negotiationAgreement() const {
	// DONE
	return _negotiationAgreement;
}


void UCP::requestItem()
{
	// DONE

	PacketHeader pckHeader;
	pckHeader.packetType = PacketType::ItemRequestToUCC;
	pckHeader.srcAgentId = id();
	pckHeader.dstAgentId = _uccLocation.agentId;

	OutputMemoryStream stream;
	pckHeader.Write(stream);

	sendPacketToHost(_uccLocation.hostIP, _uccLocation.hostPort, stream);
}

void UCP::createChildMCP(uint16_t constraintItemId)
{
	// Done
	_mcp = std::make_shared<MCP>(node(), constraintItemId, layer + 1);
	g_AgentContainer->addAgent(_mcp);
}

void UCP::destroyChildMCP()
{
	// DONE
	if (_mcp)
		_mcp->finalize();
}

void UCP::endNegotiation(TCPSocketPtr& socket, const PacketHeader& pckHeader, bool agreement)
{
	PacketHeader outPckHeader;
	outPckHeader.packetType = PacketType::ResourceNegotiationEnd;
	outPckHeader.srcAgentId = id();
	outPckHeader.dstAgentId = pckHeader.srcAgentId;
	PacketResourceNegotiationEnd pckData;
	pckData.agreement = agreement;

	OutputMemoryStream stream;
	outPckHeader.Write(stream);
	pckData.Write(stream);
	
	socket->SendPacket(stream.GetBufferPtr(), stream.GetSize());
}

void UCP::endNegotiation(bool agreement)
{
	PacketHeader pckHeader;
	pckHeader.packetType = PacketType::ResourceNegotiationEnd;
	pckHeader.srcAgentId = id();
	pckHeader.dstAgentId = _uccLocation.agentId;
	PacketResourceNegotiationEnd packetData;
	packetData.agreement = agreement;

	OutputMemoryStream stream;
	pckHeader.Write(stream);
	packetData.Write(stream);

	sendPacketToHost(_uccLocation.hostIP, _uccLocation.hostPort, stream);
}
