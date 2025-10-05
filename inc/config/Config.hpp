#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "config/ServerBlock.hpp"
#include <vector>

class Config {

	std::string	const&			_filePath;
	std::vector<ServerBlock>	_servers;

	Config();

	void		_addServer(ServerBlock const&);
	std::string	_extractFileContent();

	static void			_addTokenIf(std::string& token, std::vector<std::string>& tokens);
	static std::string	_getBlockContent(std::string const& content, size_t& index);

	public:

		Config(std::string const& filePath);

		void	parse();
		void	print() const;

		std::vector<ServerBlock> const&	getServers() const;
};

#endif

/*
function parse(file_path):
    rootContext = new Context("root")
    contextStack = [rootContext]   // stack pour gérer les nested blocks

    currentToken = ""             // buffer pour construire les mots
    tokens = []                   // tokens d'une directive

    file = open(file_path)

    while not EOF(file):
        char = readNextChar(file)

        if char is whitespace:
            if currentToken not empty:
                tokens.append(currentToken)
                currentToken = ""
            continue

        else if char == ';':
            if currentToken not empty:
                tokens.append(currentToken)
                currentToken = ""

            // fin de directive → ajouter à context courant
            currentContext = contextStack.top()
            currentContext.addDirective(tokens)
            tokens = []

        else if char == '{':
            if currentToken not empty:
                tokens.append(currentToken)
                currentToken = ""

            // début de block → créer un nouveau contexte
            blockName = tokens[0]       // ex: "server" ou "location"
            args = tokens[1:]           // ex: "/cgi-bin/"
            newContext = new Context(blockName, args)

            // ajouter le block au contexte courant
            contextStack.top().addSubContext(newContext)

            // entrer dans le nouveau block
            contextStack.push(newContext)
            tokens = []

        else if char == '}':
            if currentToken not empty:
                tokens.append(currentToken)
                currentToken = ""

            if tokens not empty:
                // fin de directive avant } ?
                contextStack.top().addDirective(tokens)
                tokens = []

            // sortir du block
            contextStack.pop()

        else if char == '#':
            // commentaire → ignorer jusqu'à fin de ligne
            skipUntil('\n')

        else:
            // caractère normal → ajouter au token courant
            currentToken += char

    return rootContext

*/
