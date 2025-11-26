#ifndef SESSION_HPP
# define SESSION_HPP

/**
 * Represents a single user session with identity, timing, and custom data
 * 
 * Each Session object contains:
 * - Private attributes: session ID (_id) and username (_username)
 * - Creation and last activity timestamps for expiration tracking
 * - Flexible key-value storage for session-specific data
 * - Logic to determine session validity based on timeout
 * 
 * This class is a data container for individual user sessions!
 */