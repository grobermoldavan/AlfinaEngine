#ifndef __ALFINA_BASE_COMMAND_BUFFER_H__
#define __ALFINA_BASE_COMMAND_BUFFER_H__

#include <type_traits>
#include <algorithm>

#include "engine/memory/stack_allocator.h"

namespace al::engine
{
    namespace cb
    {
        using DispatchFunction = void (*)(void *);
        using CommandPacket = void *;
        // ComandPacket stores :
        // CommandPacket    next packet
        // void*            dispatch fuтction
        // void*            command
        // void*            additional data

        static const size_t OFFSET_NEXT_PACKET = 0;
        static const size_t OFFSET_DISPATCH_FUNCTION = OFFSET_NEXT_PACKET + sizeof(CommandPacket);
        static const size_t OFFSET_COMMAND = OFFSET_DISPATCH_FUNCTION + sizeof(DispatchFunction);

        void* add_offset(void* ptr, ptrdiff_t offset)
        {
            return reinterpret_cast<char*>(ptr) + offset;
        }

        CommandPacket get_packet(void *command)
        {
            return reinterpret_cast<CommandPacket *>(add_offset(command, OFFSET_NEXT_PACKET - OFFSET_COMMAND));
        }

        CommandPacket get_next_packet(CommandPacket packet)
        {
            return *reinterpret_cast<CommandPacket *>(add_offset(packet, OFFSET_NEXT_PACKET));
        }

        void set_next_packet(CommandPacket packet, CommandPacket next)
        {
            *reinterpret_cast<CommandPacket *>(add_offset(packet, OFFSET_NEXT_PACKET)) = next;
        }

        void set_next_command(void *command, void *nextCommand)
        {
            set_next_packet(get_packet(command), get_packet(nextCommand));
        }

        DispatchFunction *get_dispatch_function(CommandPacket packet)
        {
            return reinterpret_cast<DispatchFunction *>(add_offset(packet, OFFSET_DISPATCH_FUNCTION));
        }

        void set_dispatch_function(CommandPacket packet, DispatchFunction function)
        {
            *get_dispatch_function(packet) = function;
        }

        DispatchFunction load_dispatch_function(CommandPacket packet)
        {
            return *get_dispatch_function(packet);
        }

        void *get_command(CommandPacket packet)
        {
            return reinterpret_cast<void *>(add_offset(packet, OFFSET_COMMAND));
        }

        template <typename CommandType>
        CommandType *get_command(CommandPacket packet)
        {
            return reinterpret_cast<CommandType *>(get_command(packet));
        }

        template <typename CommandType>
        void *get_additional_data(CommandPacket packet)
        {
            return (char *)packet + OFFSET_COMMAND + sizeof(CommandType);
        }

#if 0
        using ExampleBufferKey = uint16_t;

        ExampleBufferKey construct_example_key(/*here goes all data nessesary to make key*/ uint8_t a, uint8_t b)
        {
            return a | (b << 8);
        }

        struct ExampleCommand
        {
            static const DispatchFunction dispatchFunction;

            int value;
            int anotherValue;
        };

        void example_dispatch_function(void* command)
        {
            ExampleCommand* exampleCommand = reinterpret_cast<ExampleCommand*>(command);
            int result = exampleCommand->value + exampleCommand->anotherValue;
            printf("example result : %d\n", result);
        }

        const DispatchFunction ExampleCommand::dispatchFunction = example_dispatch_function;

        struct ExampleCommand2
        {
            static const DispatchFunction dispatchFunction;

            const char* msg;
        };

        void example_dispatch_function2(void* command)
        {
            ExampleCommand2* exampleCommand = reinterpret_cast<ExampleCommand2*>(command);
            printf("example result : %s\n", exampleCommand->msg);
        }

        const DispatchFunction ExampleCommand2::dispatchFunction = example_dispatch_function2;

        void example()
        {
            static const char* exMsg = "Example message.";
            static const char* exMsg2 = "This is an appended message.";

            CommandBuffer<ExampleBufferKey> buf;
            {
                ExampleBufferKey key = construct_example_key(10, 89);
                ExampleCommand* command = buf.add_command<ExampleCommand>(key, 0);
                command->value = 228;
                command->anotherValue = 420;
            }

            {
                ExampleBufferKey key = construct_example_key(10, 23);
                ExampleCommand* command = buf.add_command<ExampleCommand>(key, 0);
                command->value = 12;
                command->anotherValue = 42;

                ExampleCommand2* command2 = buf.append_command<ExampleCommand2>(command, 0);
                command2->msg = exMsg2;

                ExampleCommand2* command3 = buf.append_command<ExampleCommand2>(command2, 0);
                command3->msg = exMsg2;

                ExampleCommand2* command4 = buf.append_command<ExampleCommand2>(command3, 0);
                command4->msg = exMsg2;
            }
            
            {
                ExampleBufferKey key = construct_example_key(1, 90);
                ExampleCommand2* command = buf.add_command<ExampleCommand2>(key, 0);
                command->msg = exMsg;
            }
            
            buf.sort();
            buf.dispatch();
        }
#endif

    } // namespace cb

    template <typename Key, class = typename std::enable_if_t<std::is_integral<Key>::value>>
    class CommandBuffer
    {
    public:
        //void *allocate(size_t size)
        //{
        //    static size_t ptr = 0;
        //    static uint8_t memory[1024];
        //
        //    void *res = memory + ptr;
        //    ptr += size;
        //    return res;
        //}

        void init_allocator(size_t sizeBytes, std::function<uint8_t* (size_t)> allocate) noexcept
        {
            allocator.init(sizeBytes, allocate)
        }

        template <typename CommandType>
        [[nodiscard]] CommandType *add_command(Key key, size_t additionalDataSizeBytes) noexcept
        {
            cb::CommandPacket packet = reinterpret_cast<cb::CommandPacket>(allocator.allocate(cb::OFFSET_COMMAND + sizeof(CommandType) + additionalDataSizeBytes));
            cb::set_next_packet(packet, nullptr);
            cb::set_dispatch_function(packet, CommandType::dispatchFunction);

            keys[currentId] = key;
            packets[currentId++] = packet;

            return cb::get_command<CommandType>(packet);
        }

        template <typename CommandType, typename OtherCommandType>
        [[nodiscard]] CommandType *append_command(OtherCommandType command, size_t additionalDataSizeBytes) noexcept
        {
            cb::CommandPacket packet = reinterpret_cast<cb::CommandPacket>(allocate(cb::OFFSET_COMMAND + sizeof(CommandType) + additionalDataSizeBytes));
            cb::set_next_packet(packet, nullptr);
            cb::set_dispatch_function(packet, CommandType::dispatchFunction);
            cb::set_next_packet(cb::get_packet(command), packet);

            return cb::get_command<CommandType>(packet);
        }

        void sort() noexcept
        {
            // @TODO : implement sort
        }

        void dispatch() noexcept
        {
            for (size_t i = 0; i < currentId; i++)
            {
                cb::CommandPacket packet = packets[i];
                cb::load_dispatch_function(packet)(cb::get_command(packet));
                while (packet = cb::get_next_packet(packet))
                    cb::load_dispatch_function(packet)(cb::get_command(packet));
            }

            currentId = 0;
            allocator.clear();
        }

    private:
        static constexpr const size_t MAX_COMMANDS = 128;

        Key keys[MAX_COMMANDS];
        cb::CommandPacket packets[MAX_COMMANDS];
        size_t currentId = 0;
        StackAllocator allocator;
    };

} // namespace al::engine

#endif