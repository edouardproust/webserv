#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "ServerBlock.hpp"
#include <vector>

class Config {

	std::vector<ServerBlock>   servers;

	public:

		void	parse(std::string&);
};

#endif

/*

function parseConfig(file_path):
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
