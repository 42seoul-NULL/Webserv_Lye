void		Response::generateErrorPage(int status)
{
	this->status = status;

	this->body.clear();
	this->body += "<html>\r\n";
	this->body += "<head>\r\n";
	this->body += "<title>" + ft_itoa(this->status) + " " + Manager::getInstance()->getStatusCode().find(ft_itoa(this->status))->second + "</title>\r\n";
	this->body += "</head>\r\n";
	this->body += "<body bgcolor=\"white\">\r\n"
	this->body += "<center>\r\n";
	this->body += "<h1>" + ft_itoa(this->status) + " " + Manager::getInstance()->getStatusCode().find(ft_itoa(this->status))->second + "</h1>\r\n";
	this->body += "</center>\r\n";
	this->body += "<hr>\r\n";
	this->body += "<center>HyeonSkkiDashi/1.0</center>\r\n";
	this->body += "</body>\r\n";
	this->body += "</html>";
}